#include "FStaticMeshNaniteSuitabilityRule.h"
#include "Analysis/FAssetAnalysisResult.h"
#include "FPipelineGuardianSettings.h"
#include "PipelineGuardian.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSourceData.h"
#include "StaticMeshResources.h"
#include "Misc/MessageDialog.h"

FStaticMeshNaniteSuitabilityRule::FStaticMeshNaniteSuitabilityRule()
{
}

bool FStaticMeshNaniteSuitabilityRule::Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults)
{
	UStaticMesh* StaticMesh = Cast<UStaticMesh>(Asset);
	if (!StaticMesh)
	{
		return false;
	}

	// Get settings
	const UPipelineGuardianSettings* Settings = GetDefault<UPipelineGuardianSettings>();
	if (!Settings || !Settings->bEnableStaticMeshNaniteSuitabilityRule)
	{
		return false;
	}

	// Get triangle count
	int32 TriangleCount = 0;
	if (StaticMesh->GetRenderData() && StaticMesh->GetRenderData()->LODResources.Num() > 0)
	{
		TriangleCount = StaticMesh->GetRenderData()->LODResources[0].GetNumTriangles();
	}

	// Check Nanite suitability
	bool ShouldUseNanite = this->ShouldUseNanite(StaticMesh, Settings->NaniteSuitabilityThreshold, Settings->NaniteDisableThreshold);
	bool ShouldDisable = this->ShouldDisableNanite(StaticMesh, Settings->NaniteDisableThreshold);
	bool HasNaniteEnabled = StaticMesh->NaniteSettings.bEnabled;

	// Determine if there's an issue
	bool HasIssue = false;
	EAssetIssueSeverity Severity = Settings->NaniteSuitabilityIssueSeverity;

	if (ShouldUseNanite && !HasNaniteEnabled)
	{
		HasIssue = true;
		Severity = EAssetIssueSeverity::Warning; // High-poly mesh without Nanite
	}
	else if (ShouldDisable && HasNaniteEnabled)
	{
		HasIssue = true;
		Severity = EAssetIssueSeverity::Info; // Low-poly mesh with Nanite (performance waste)
	}

	if (HasIssue)
	{
		FAssetAnalysisResult Result;
		Result.Asset = FAssetData(StaticMesh);
		Result.RuleID = GetRuleID();
		Result.Severity = Severity;
		Result.Description = FText::FromString(GenerateNaniteSuitabilityDescription(StaticMesh, ShouldUseNanite, ShouldDisable, TriangleCount));

		// Add fix action if auto-fix is enabled
		if (Settings->bAllowNaniteSuitabilityAutoFix && CanSafelyOptimizeNanite(StaticMesh))
		{
			Result.FixAction = FSimpleDelegate::CreateLambda([this, StaticMesh, ShouldUseNanite]()
			{
				if (OptimizeNaniteSettings(StaticMesh, ShouldUseNanite))
				{
					UE_LOG(LogPipelineGuardian, Log, TEXT("Successfully optimized Nanite settings for %s"), *StaticMesh->GetName());
				}
				else
				{
					UE_LOG(LogPipelineGuardian, Warning, TEXT("Failed to optimize Nanite settings for %s"), *StaticMesh->GetName());
				}
			});
		}

		OutResults.Add(Result);
		return true;
	}

	return false;
}

FName FStaticMeshNaniteSuitabilityRule::GetRuleID() const
{
	return TEXT("SM_NaniteSuitability");
}

FText FStaticMeshNaniteSuitabilityRule::GetRuleDescription() const
{
	return FText::FromString(TEXT("Checks if static meshes should use Nanite based on polygon count for optimal performance and quality."));
}

bool FStaticMeshNaniteSuitabilityRule::ShouldUseNanite(const UStaticMesh* StaticMesh, int32 SuitabilityThreshold, int32 DisableThreshold) const
{
	if (!StaticMesh || !StaticMesh->GetRenderData() || StaticMesh->GetRenderData()->LODResources.Num() == 0)
	{
		return false;
	}

	int32 TriangleCount = StaticMesh->GetRenderData()->LODResources[0].GetNumTriangles();
	return TriangleCount >= SuitabilityThreshold;
}

bool FStaticMeshNaniteSuitabilityRule::ShouldDisableNanite(const UStaticMesh* StaticMesh, int32 DisableThreshold) const
{
	if (!StaticMesh || !StaticMesh->GetRenderData() || StaticMesh->GetRenderData()->LODResources.Num() == 0)
	{
		return false;
	}

	int32 TriangleCount = StaticMesh->GetRenderData()->LODResources[0].GetNumTriangles();
	return TriangleCount <= DisableThreshold;
}

FString FStaticMeshNaniteSuitabilityRule::GenerateNaniteSuitabilityDescription(const UStaticMesh* StaticMesh, bool ShouldUseNanite, bool ShouldDisable, int32 TriangleCount) const
{
	const UPipelineGuardianSettings* Settings = GetDefault<UPipelineGuardianSettings>();
	bool HasNaniteEnabled = StaticMesh->NaniteSettings.bEnabled;

	if (ShouldUseNanite && !HasNaniteEnabled)
	{
		return FString::Printf(TEXT("High-poly mesh (%d triangles) should use Nanite for optimal performance. Current: Nanite Disabled. Recommended: Enable Nanite."), TriangleCount);
	}
	else if (ShouldDisable && HasNaniteEnabled)
	{
		return FString::Printf(TEXT("Low-poly mesh (%d triangles) has Nanite enabled unnecessarily. Current: Nanite Enabled. Recommended: Disable Nanite for better performance."), TriangleCount);
	}

	return FString::Printf(TEXT("Nanite suitability check failed for %s (%d triangles)"), *StaticMesh->GetName(), TriangleCount);
}

bool FStaticMeshNaniteSuitabilityRule::OptimizeNaniteSettings(UStaticMesh* StaticMesh, bool ShouldUseNanite) const
{
	if (!StaticMesh)
	{
		return false;
	}

	UE_LOG(LogPipelineGuardian, Log, TEXT("Optimizing Nanite settings for %s: %s"), *StaticMesh->GetName(), ShouldUseNanite ? TEXT("Enable") : TEXT("Disable"));

	// Modify Nanite settings
	StaticMesh->NaniteSettings.bEnabled = ShouldUseNanite;

	// If enabling Nanite, set reasonable defaults
	if (ShouldUseNanite)
	{
		StaticMesh->NaniteSettings.bPreserveArea = true;
		StaticMesh->NaniteSettings.bExplicitTangents = false;
		// Note: bExplicitNormals is not available in UE 5.5
	}

	// Mark for rebuild
	StaticMesh->Build(false);
	StaticMesh->MarkPackageDirty();
	StaticMesh->PostEditChange();

	UE_LOG(LogPipelineGuardian, Log, TEXT("Successfully %s Nanite for %s"), ShouldUseNanite ? TEXT("enabled") : TEXT("disabled"), *StaticMesh->GetName());
	return true;
}

bool FStaticMeshNaniteSuitabilityRule::CanSafelyOptimizeNanite(const UStaticMesh* StaticMesh) const
{
	if (!StaticMesh)
	{
		return false;
	}

	// Check if mesh has valid geometry
	if (!StaticMesh->GetRenderData() || StaticMesh->GetRenderData()->LODResources.Num() == 0)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("Cannot optimize Nanite for %s: No valid geometry"), *StaticMesh->GetName());
		return false;
	}

	// Check if mesh is not too complex for analysis
	const FStaticMeshLODResources* LODResource = &StaticMesh->GetRenderData()->LODResources[0];
	int32 TriangleCount = LODResource->GetNumTriangles();
	
	// Don't auto-optimize for extremely complex meshes (more than 1M triangles)
	if (TriangleCount > 1000000)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("Cannot auto-optimize Nanite for %s: Too complex (%d triangles)"), 
			*StaticMesh->GetName(), TriangleCount);
		return false;
	}

	return true;
} 