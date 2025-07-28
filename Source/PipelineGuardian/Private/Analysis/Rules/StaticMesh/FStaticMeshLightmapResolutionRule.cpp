#include "FStaticMeshLightmapResolutionRule.h"
#include "Analysis/FAssetAnalysisResult.h"
#include "FPipelineGuardianSettings.h"
#include "PipelineGuardian.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSourceData.h"
#include "StaticMeshResources.h"
#include "Misc/MessageDialog.h"

FStaticMeshLightmapResolutionRule::FStaticMeshLightmapResolutionRule()
{
}

bool FStaticMeshLightmapResolutionRule::Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults)
{
	UStaticMesh* StaticMesh = Cast<UStaticMesh>(Asset);
	if (!StaticMesh)
	{
		return false;
	}

	// Get settings
	const UPipelineGuardianSettings* Settings = GetDefault<UPipelineGuardianSettings>();
	if (!Settings || !Settings->bEnableStaticMeshLightmapResolutionRule)
	{
		return false;
	}

	// Check lightmap resolution
	int32 CurrentResolution = 0;
	bool HasInappropriateResolution = HasInappropriateLightmapResolution(StaticMesh, Settings->LightmapResolutionMin, Settings->LightmapResolutionMax, CurrentResolution);

	if (HasInappropriateResolution)
	{
		FAssetAnalysisResult Result;
		Result.Asset = FAssetData(StaticMesh);
		Result.RuleID = GetRuleID();
		Result.Severity = Settings->LightmapResolutionIssueSeverity;
		Result.Description = FText::FromString(GenerateLightmapResolutionDescription(StaticMesh, CurrentResolution, Settings->LightmapResolutionMin, Settings->LightmapResolutionMax));

		// Add fix action if auto-fix is enabled
		if (Settings->bAllowLightmapResolutionAutoFix && CanSafelySetLightmapResolution(StaticMesh))
		{
			Result.FixAction = FSimpleDelegate::CreateLambda([this, StaticMesh, Settings]()
			{
				if (SetOptimalLightmapResolution(StaticMesh, Settings->LightmapResolutionMin, Settings->LightmapResolutionMax))
				{
					UE_LOG(LogPipelineGuardian, Log, TEXT("Successfully set optimal lightmap resolution for %s"), *StaticMesh->GetName());
				}
				else
				{
					UE_LOG(LogPipelineGuardian, Warning, TEXT("Failed to set lightmap resolution for %s"), *StaticMesh->GetName());
				}
			});
		}

		OutResults.Add(Result);
		return true;
	}

	return false;
}

FName FStaticMeshLightmapResolutionRule::GetRuleID() const
{
	return TEXT("SM_LightmapResolution");
}

FText FStaticMeshLightmapResolutionRule::GetRuleDescription() const
{
	return FText::FromString(TEXT("Checks if static meshes have appropriate lightmap resolution settings for optimal lighting quality and performance."));
}

bool FStaticMeshLightmapResolutionRule::HasInappropriateLightmapResolution(const UStaticMesh* StaticMesh, int32 MinResolution, int32 MaxResolution, int32& OutCurrentResolution) const
{
	if (!StaticMesh)
	{
		return false;
	}

	// Get current lightmap resolution
	int32 CurrentResolution = StaticMesh->GetLightMapResolution();
	OutCurrentResolution = CurrentResolution;

	// Check if resolution is outside the acceptable range
	return CurrentResolution < MinResolution || CurrentResolution > MaxResolution;
}

FString FStaticMeshLightmapResolutionRule::GenerateLightmapResolutionDescription(const UStaticMesh* StaticMesh, int32 CurrentResolution, int32 MinResolution, int32 MaxResolution) const
{
	if (CurrentResolution < MinResolution)
	{
		return FString::Printf(TEXT("Static mesh %s has low lightmap resolution (%d < %d minimum). This may result in poor lighting quality."), 
			*StaticMesh->GetName(), CurrentResolution, MinResolution);
	}
	else if (CurrentResolution > MaxResolution)
	{
		return FString::Printf(TEXT("Static mesh %s has high lightmap resolution (%d > %d maximum). This may impact performance unnecessarily."), 
			*StaticMesh->GetName(), CurrentResolution, MaxResolution);
	}

	return FString::Printf(TEXT("Lightmap resolution check failed for %s"), *StaticMesh->GetName());
}

bool FStaticMeshLightmapResolutionRule::SetOptimalLightmapResolution(UStaticMesh* StaticMesh, int32 MinResolution, int32 MaxResolution) const
{
	if (!StaticMesh)
	{
		return false;
	}

	UE_LOG(LogPipelineGuardian, Log, TEXT("Setting optimal lightmap resolution for %s"), *StaticMesh->GetName());

	// Calculate optimal resolution based on mesh complexity
	int32 OptimalResolution = CalculateOptimalLightmapResolution(StaticMesh, MinResolution, MaxResolution);

	// Set the lightmap resolution
	StaticMesh->SetLightMapResolution(OptimalResolution);

	// Mark for rebuild
	StaticMesh->Build(false);
	StaticMesh->MarkPackageDirty();
	StaticMesh->PostEditChange();

	UE_LOG(LogPipelineGuardian, Log, TEXT("Successfully set lightmap resolution to %d for %s"), OptimalResolution, *StaticMesh->GetName());
	return true;
}

bool FStaticMeshLightmapResolutionRule::CanSafelySetLightmapResolution(const UStaticMesh* StaticMesh) const
{
	if (!StaticMesh)
	{
		return false;
	}

	// Check if mesh has valid geometry
	if (!StaticMesh->GetRenderData() || StaticMesh->GetRenderData()->LODResources.Num() == 0)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("Cannot set lightmap resolution for %s: No valid geometry"), *StaticMesh->GetName());
		return false;
	}

	// Check if mesh is not too complex for lightmap resolution adjustment
	const FStaticMeshLODResources* LODResource = &StaticMesh->GetRenderData()->LODResources[0];
	int32 TriangleCount = LODResource->GetNumTriangles();
	
	// Don't auto-adjust for extremely complex meshes (more than 1M triangles)
	if (TriangleCount > PipelineGuardianConstants::MAX_TRIANGLE_COUNT_FOR_LIGHTMAP_RESOLUTION)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("Cannot auto-adjust lightmap resolution for %s: Too complex (%d triangles)"), 
			*StaticMesh->GetName(), TriangleCount);
		return false;
	}

	return true;
}

int32 FStaticMeshLightmapResolutionRule::CalculateOptimalLightmapResolution(const UStaticMesh* StaticMesh, int32 MinResolution, int32 MaxResolution) const
{
	if (!StaticMesh || !StaticMesh->GetRenderData() || StaticMesh->GetRenderData()->LODResources.Num() == 0)
	{
		return MinResolution;
	}

	// Get mesh complexity metrics
	const FStaticMeshLODResources* LODResource = &StaticMesh->GetRenderData()->LODResources[0];
	int32 TriangleCount = LODResource->GetNumTriangles();
	FVector BoundsSize = StaticMesh->GetBoundingBox().GetSize();
	float SurfaceArea = BoundsSize.X * BoundsSize.Y * 2.0f + BoundsSize.Y * BoundsSize.Z * 2.0f + BoundsSize.Z * BoundsSize.X * 2.0f;

	// Calculate optimal resolution based on complexity
	int32 OptimalResolution = MinResolution;

	// Adjust based on triangle count
	if (TriangleCount > 10000)
	{
		OptimalResolution = FMath::Max(OptimalResolution, 8);  // 256x256
	}
	if (TriangleCount > 50000)
	{
		OptimalResolution = FMath::Max(OptimalResolution, 10); // 1024x1024
	}
	if (TriangleCount > 100000)
	{
		OptimalResolution = FMath::Max(OptimalResolution, 12); // 4096x4096
	}

	// Adjust based on surface area
	if (SurfaceArea > 10000.0f)
	{
		OptimalResolution = FMath::Max(OptimalResolution, 8);
	}
	if (SurfaceArea > 100000.0f)
	{
		OptimalResolution = FMath::Max(OptimalResolution, 10);
	}

	// Clamp to valid range
	return FMath::Clamp(OptimalResolution, MinResolution, MaxResolution);
} 