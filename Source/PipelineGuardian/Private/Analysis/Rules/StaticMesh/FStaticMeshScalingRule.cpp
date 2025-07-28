#include "FStaticMeshScalingRule.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "Misc/MessageDialog.h"
#include "PipelineGuardian.h"

FStaticMeshScalingRule::FStaticMeshScalingRule()
{
}

bool FStaticMeshScalingRule::Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults)
{
	UStaticMesh* StaticMesh = Cast<UStaticMesh>(Asset);
	if (!StaticMesh || !Profile)
	{
		return false;
	}

	// Get settings
	const UPipelineGuardianSettings* Settings = GetDefault<UPipelineGuardianSettings>();
	if (!Settings)
	{
		return false;
	}

	bool HasIssues = false;

	// Check for non-uniform scaling
	if (Settings->bEnableNonUniformScaleDetection)
	{
		FVector Scale;
		bool HasNonUniform = this->HasNonUniformScale(StaticMesh, Settings->NonUniformScaleWarningRatio, Scale);
		if (HasNonUniform)
		{
			FAssetAnalysisResult Result;
			Result.Asset = FAssetData(StaticMesh);
			Result.RuleID = FName(TEXT("SM_NonUniformScale"));
			Result.Severity = Settings->NonUniformScaleIssueSeverity;
			Result.Description = FText::FromString(GenerateNonUniformScaleDescription(StaticMesh, Scale, Settings->NonUniformScaleWarningRatio));
			OutResults.Add(Result);
			HasIssues = true;
		}
	}

	// Check for zero scale components
	if (Settings->bEnableZeroScaleDetection)
	{
		TArray<FString> ZeroScaleAxes;
		bool HasZeroScale = this->HasZeroScale(StaticMesh, Settings->ZeroScaleThreshold, ZeroScaleAxes);
		if (HasZeroScale)
		{
			FAssetAnalysisResult Result;
			Result.Asset = FAssetData(StaticMesh);
			Result.RuleID = FName(TEXT("SM_ZeroScale"));
			Result.Severity = Settings->ZeroScaleIssueSeverity;
			Result.Description = FText::FromString(GenerateZeroScaleDescription(StaticMesh, ZeroScaleAxes));
			
			// Only provide fix action if it's safe
			if (CanSafelyFixScaling(StaticMesh))
			{
				Result.FixAction = FSimpleDelegate::CreateLambda([this, StaticMesh, ZeroScaleAxes]() {
					if (FixZeroScale(const_cast<UStaticMesh*>(StaticMesh), ZeroScaleAxes)) 
					{ 
						UE_LOG(LogPipelineGuardian, Log, TEXT("Successfully fixed zero scale for %s"), *StaticMesh->GetName()); 
					} 
					else 
					{ 
						UE_LOG(LogPipelineGuardian, Warning, TEXT("Failed to fix zero scale for %s"), *StaticMesh->GetName()); 
					}
				});
			}
			
			OutResults.Add(Result);
			HasIssues = true;
		}
	}

	// Check for extreme scale values
	if (Settings->bEnableAssetTypeSpecificPivotRules)
	{
		TArray<FString> ScaleIssues;
		bool HasExtremeScale = this->HasExtremeScaleValues(StaticMesh, ScaleIssues);
		if (HasExtremeScale)
		{
			FAssetAnalysisResult Result;
			Result.Asset = FAssetData(StaticMesh);
			Result.RuleID = FName(TEXT("SM_ExtremeScale"));
			Result.Severity = Settings->TransformPivotIssueSeverity;
			Result.Description = FText::FromString(GenerateExtremeScaleDescription(StaticMesh, ScaleIssues));
			OutResults.Add(Result);
			HasIssues = true;
		}
	}

	return HasIssues;
}

FName FStaticMeshScalingRule::GetRuleID() const
{
	return TEXT("SM_Scaling");
}

FText FStaticMeshScalingRule::GetRuleDescription() const
{
	return FText::FromString(TEXT("Checks for scaling issues in static meshes including non-uniform scaling, zero scale, and extreme values."));
}

bool FStaticMeshScalingRule::HasNonUniformScale(const UStaticMesh* StaticMesh, float WarningRatio, FVector& OutScale) const
{
	if (!StaticMesh)
	{
		return false;
	}

	// Get the actual scale from the mesh's transform
	// For static meshes, we need to check the source model's transform
	if (StaticMesh->GetSourceModel(0).BuildSettings.bRecomputeNormals)
	{
		// Check if there are any transform overrides in the source model
		// This is a simplified check - in practice, you might need to access the actual imported transform
		OutScale = FVector(1.0f, 1.0f, 1.0f);
		return false;
	}

	// For now, we'll check the bounding box size ratios as a proxy for scale
	FBox BoundingBox = StaticMesh->GetBoundingBox();
	FVector Size = BoundingBox.GetSize();
	
	// Check if any dimension is significantly different from others
	float MaxSize = FMath::Max3(Size.X, Size.Y, Size.Z);
	float MinSize = FMath::Min3(Size.X, Size.Y, Size.Z);
	
	if (MaxSize > 0.0f && MinSize > 0.0f)
	{
		float Ratio = MaxSize / MinSize;
		OutScale = Size;
		return Ratio > WarningRatio;
	}

	return false;
}

bool FStaticMeshScalingRule::HasZeroScale(const UStaticMesh* StaticMesh, float Threshold, TArray<FString>& OutZeroScaleAxes) const
{
	if (!StaticMesh)
	{
		return false;
	}

	// Check the bounding box size for zero or near-zero dimensions
	FBox BoundingBox = StaticMesh->GetBoundingBox();
	FVector Size = BoundingBox.GetSize();
	
	bool HasZeroScale = false;
	
	if (Size.X < Threshold)
	{
		OutZeroScaleAxes.Add(TEXT("X"));
		HasZeroScale = true;
	}
	
	if (Size.Y < Threshold)
	{
		OutZeroScaleAxes.Add(TEXT("Y"));
		HasZeroScale = true;
	}
	
	if (Size.Z < Threshold)
	{
		OutZeroScaleAxes.Add(TEXT("Z"));
		HasZeroScale = true;
	}
	
	return HasZeroScale;
}

bool FStaticMeshScalingRule::HasExtremeScaleValues(const UStaticMesh* StaticMesh, TArray<FString>& OutIssues) const
{
	if (!StaticMesh)
	{
		return false;
	}

	// Check bounding box size for extreme values
	FBox BoundingBox = StaticMesh->GetBoundingBox();
	FVector Size = BoundingBox.GetSize();
	
	bool HasExtremeValues = false;
	
	// Check for very large scale values (>1000 units)
	if (Size.X > PipelineGuardianConstants::MAX_SCALE_THRESHOLD || Size.Y > PipelineGuardianConstants::MAX_SCALE_THRESHOLD || Size.Z > PipelineGuardianConstants::MAX_SCALE_THRESHOLD)
	{
		OutIssues.Add(TEXT("Extreme scale values (>1000 units)"));
		HasExtremeValues = true;
	}
	
	// Check for very small scale values (<0.001 units)
	if (Size.X < PipelineGuardianConstants::MIN_SCALE_THRESHOLD || Size.Y < PipelineGuardianConstants::MIN_SCALE_THRESHOLD || Size.Z < PipelineGuardianConstants::MIN_SCALE_THRESHOLD)
	{
		OutIssues.Add(TEXT("Very small scale values (<0.001 units)"));
		HasExtremeValues = true;
	}
	
	return HasExtremeValues;
}

FString FStaticMeshScalingRule::GenerateNonUniformScaleDescription(const UStaticMesh* StaticMesh, const FVector& Scale, float WarningRatio) const
{
	return FString::Printf(TEXT("Static mesh %s has non-uniform scaling (X:%.2f, Y:%.2f, Z:%.2f). This can cause rendering artifacts and should be uniform for consistent results."), 
		*StaticMesh->GetName(), Scale.X, Scale.Y, Scale.Z);
}

FString FStaticMeshScalingRule::GenerateZeroScaleDescription(const UStaticMesh* StaticMesh, const TArray<FString>& ZeroScaleAxes) const
{
	FString AxesText = FString::Join(ZeroScaleAxes, TEXT(", "));
	return FString::Printf(TEXT("Static mesh %s has zero or near-zero scale on axes: %s. This can cause rendering and collision issues."), 
		*StaticMesh->GetName(), *AxesText);
}

FString FStaticMeshScalingRule::GenerateExtremeScaleDescription(const UStaticMesh* StaticMesh, const TArray<FString>& Issues) const
{
	FString IssuesText = FString::Join(Issues, TEXT(", "));
	return FString::Printf(TEXT("Static mesh %s has extreme scale values: %s. These should be normalized before importing."), 
		*StaticMesh->GetName(), *IssuesText);
}

bool FStaticMeshScalingRule::CanSafelyFixScaling(const UStaticMesh* StaticMesh) const
{
	if (!StaticMesh)
	{
		return false;
	}

	// Limit auto-fix to meshes under 500k triangles
	const FStaticMeshLODResources* LODResource = &StaticMesh->GetRenderData()->LODResources[0];
	int32 TriangleCount = LODResource->GetNumTriangles();
	
	return TriangleCount <= 500000;
}

bool FStaticMeshScalingRule::FixZeroScale(UStaticMesh* StaticMesh, const TArray<FString>& AxesToFix) const
{
	if (!StaticMesh)
	{
		return false;
	}

	UE_LOG(LogPipelineGuardian, Warning, TEXT("Auto-fix for zero scale not implemented for %s. Please fix manually in your DCC tool."), *StaticMesh->GetName());
	
	// Mark mesh as dirty to ensure it gets saved
	StaticMesh->MarkPackageDirty();
	
	return false; // Return false since we can't actually fix it automatically
} 