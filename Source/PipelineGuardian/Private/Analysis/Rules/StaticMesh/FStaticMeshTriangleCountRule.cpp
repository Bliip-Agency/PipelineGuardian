#include "FStaticMeshTriangleCountRule.h"
#include "Analysis/FAssetAnalysisResult.h"
#include "Analysis/FPipelineGuardianProfile.h"
#include "FPipelineGuardianSettings.h"
#include "Engine/StaticMesh.h"
#include "PipelineGuardian.h"
#include "StaticMeshResources.h"
#include "Engine/StaticMeshSourceData.h"
#include "UObject/Package.h"

// For LOD generation using the same pattern as FStaticMeshLODMissingRule
#include "MeshUtilities.h"
#include "IMeshReductionManagerModule.h"
#include "IMeshReductionInterfaces.h"
#include "Modules/ModuleManager.h"
#include "Misc/MessageDialog.h"
#include "Engine/StaticMeshSourceData.h"
#include "FPipelineGuardianSettings.h"

#define LOCTEXT_NAMESPACE "PipelineGuardian"

FStaticMeshTriangleCountRule::FStaticMeshTriangleCountRule()
{
	// Constructor - rule ready for use
}

bool FStaticMeshTriangleCountRule::Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults)
{
	UStaticMesh* StaticMesh = Cast<UStaticMesh>(Asset);
	if (!StaticMesh || !Profile)
	{
		return false;
	}

	// Get rule configuration from profile
	const FPipelineGuardianRuleConfig* RuleConfig = Profile->GetRuleConfigPtr(GetRuleID());
	if (!RuleConfig || !RuleConfig->bEnabled)
	{
		return false;
	}

	const UPipelineGuardianSettings* Settings = GetDefault<UPipelineGuardianSettings>();
	if (!Settings)
	{
		UE_LOG(LogPipelineGuardian, Error, TEXT("FStaticMeshTriangleCountRule: Could not get Pipeline Guardian settings"));
		return false;
	}
	
	if (!Settings->bEnableStaticMeshTriangleCountRule)
	{
		return false;
	}
	
	int32 CurrentTriangleCount = GetTriangleCount(StaticMesh, 0);
	if (CurrentTriangleCount == 0)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("FStaticMeshTriangleCountRule: %s has zero triangles in LOD0"), *StaticMesh->GetName());
		return false;
	}
	
	int32 BaseThreshold = Settings->TriangleCountBaseThreshold;
	float WarningPercentage = Settings->TriangleCountWarningPercentage;
	float ErrorPercentage = Settings->TriangleCountErrorPercentage;
	
	int32 WarningThreshold = BaseThreshold + FMath::RoundToInt(BaseThreshold * WarningPercentage / 100.0f);
	int32 ErrorThreshold = BaseThreshold + FMath::RoundToInt(BaseThreshold * ErrorPercentage / 100.0f);

	EAssetIssueSeverity Severity = DetermineSeverity(CurrentTriangleCount, WarningThreshold, ErrorThreshold);
	
	if (Severity != EAssetIssueSeverity::Info) // Info means no issue
	{
		FAssetAnalysisResult Result;
		Result.RuleID = GetRuleID();
		Result.Asset = FAssetData(StaticMesh);
		Result.Severity = Severity;
		Result.Description = FText::FromString(GenerateTriangleCountDescription(StaticMesh, CurrentTriangleCount, 
			BaseThreshold, (Severity == EAssetIssueSeverity::Warning) ? WarningPercentage : ErrorPercentage, Severity));
		Result.FilePath = FText::FromString(StaticMesh->GetPackage()->GetName());

		// Note: No fix action - Triangle count reduction should be done in external 3D tools
		// to preserve mesh quality, UVs, and shape integrity

		OutResults.Add(Result);
		
		UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshTriangleCountRule::Check: Triangle count issue for %s - %d triangles (thresholds: %d/%d, percentages: %.1f%%/%.1f%%)"), 
			*StaticMesh->GetName(), CurrentTriangleCount, WarningThreshold, ErrorThreshold, WarningPercentage, ErrorPercentage);

		return true;
	}

	return false;
}

FName FStaticMeshTriangleCountRule::GetRuleID() const
{
	return FName(TEXT("SM_TriangleCount"));
}

FText FStaticMeshTriangleCountRule::GetRuleDescription() const
{
	return LOCTEXT("StaticMeshTriangleCountRule_Description", 
		"Checks if static meshes exceed performance-friendly triangle count limits for LOD0. "
		"High triangle counts can impact rendering performance, especially on lower-end devices.");
}

int32 FStaticMeshTriangleCountRule::GetTriangleCount(const UStaticMesh* StaticMesh, int32 LODIndex) const
{
	if (!StaticMesh || !StaticMesh->GetRenderData() || LODIndex >= StaticMesh->GetNumLODs())
	{
		return 0;
	}

	const FStaticMeshLODResources& LODResource = StaticMesh->GetRenderData()->LODResources[LODIndex];
	return LODResource.GetNumTriangles();
}

EAssetIssueSeverity FStaticMeshTriangleCountRule::DetermineSeverity(int32 TriangleCount, int32 WarningThreshold, int32 ErrorThreshold) const
{
	if (TriangleCount >= ErrorThreshold)
	{
		return EAssetIssueSeverity::Error;
	}
	else if (TriangleCount >= WarningThreshold)
	{
		return EAssetIssueSeverity::Warning;
	}
	
	return EAssetIssueSeverity::Info; // No issue
}

// Note: Auto-fix methods removed - Triangle count reduction should be done in external 3D tools
// to preserve mesh quality, UVs, and shape integrity

FString FStaticMeshTriangleCountRule::GenerateTriangleCountDescription(const UStaticMesh* StaticMesh, int32 TriangleCount, int32 BaseThreshold, float PercentageThreshold, EAssetIssueSeverity Severity) const
{
	// Calculate how much the mesh exceeds the base threshold
	float ExcessPercentage = ((float)(TriangleCount - BaseThreshold) / (float)BaseThreshold) * 100.0f;

	FString SeverityText = (Severity == EAssetIssueSeverity::Error) ? TEXT("CRITICAL") : TEXT("WARNING");
	
	return FString::Printf(
		TEXT("%s: Triangle count %d exceeds base threshold %d by %.1f%% (threshold: %.1f%%). ")
		TEXT("High triangle counts impact rendering performance, especially on mobile devices and VR. ")
		TEXT("Optimize mesh in external 3D tools (Blender, Maya, 3ds Max) to preserve UVs and shape quality."),
		*SeverityText,
		TriangleCount,
		BaseThreshold,
		ExcessPercentage,
		PercentageThreshold
	);
}

#undef LOCTEXT_NAMESPACE 