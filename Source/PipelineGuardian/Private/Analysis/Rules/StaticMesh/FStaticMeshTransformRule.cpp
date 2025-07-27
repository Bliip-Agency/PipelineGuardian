#include "FStaticMeshTransformRule.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "Misc/MessageDialog.h"
#include "PipelineGuardian.h"

FStaticMeshTransformRule::FStaticMeshTransformRule()
{
}

bool FStaticMeshTransformRule::Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults)
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

	// Check for problematic pivot offset relative to mesh bounds
	if (Settings->bEnableStaticMeshTransformPivotRule)
	{
		UE_LOG(LogPipelineGuardian, Verbose, TEXT("TransformPivot Rule: WarningDistance=%.2f, ErrorDistance=%.2f"), 
			Settings->TransformPivotWarningDistance, Settings->TransformPivotErrorDistance);
		
		FVector PivotOffset;
		bool HasProblematicPivot = this->HasProblematicPivot(StaticMesh, Settings->TransformPivotWarningDistance, Settings->TransformPivotErrorDistance, PivotOffset);
		if (HasProblematicPivot)
		{
			FAssetAnalysisResult Result;
			Result.Asset = FAssetData(StaticMesh);
			Result.RuleID = FName(TEXT("SM_TransformPivot"));
			Result.Severity = Settings->TransformPivotIssueSeverity;
			Result.Description = FText::FromString(GenerateTransformPivotDescription(StaticMesh, PivotOffset, Settings->TransformPivotWarningDistance, Settings->TransformPivotErrorDistance));
			
			// No auto-fix for transform pivot - use external DCC tools for pivot adjustments
			
			OutResults.Add(Result);
			HasIssues = true;
		}
	}

	// Check for unapplied DCC transformations
	if (Settings->bEnableAssetTypeSpecificPivotRules)
	{
		TArray<FString> DCCIssues;
		bool HasDCCIssues = this->HasUnappliedDCCTransformations(StaticMesh, DCCIssues);
		if (HasDCCIssues)
		{
			FAssetAnalysisResult Result;
			Result.Asset = FAssetData(StaticMesh);
			Result.RuleID = FName(TEXT("SM_DCCTransform"));
			Result.Severity = Settings->TransformPivotIssueSeverity;
			Result.Description = FText::FromString(GenerateDCCTransformDescription(StaticMesh, DCCIssues));
			OutResults.Add(Result);
			HasIssues = true;
		}
	}

	return HasIssues;
}

FName FStaticMeshTransformRule::GetRuleID() const
{
	return TEXT("SM_Transform");
}

FText FStaticMeshTransformRule::GetRuleDescription() const
{
	return FText::FromString(TEXT("Checks for transform and pivot issues in static meshes including off-origin pivots and unapplied DCC transformations."));
}

bool FStaticMeshTransformRule::HasProblematicPivot(const UStaticMesh* StaticMesh, float WarningDistance, float ErrorDistance, FVector& OutPivotOffset) const
{
	if (!StaticMesh)
	{
		return false;
	}

	// Get the mesh bounds and center
	FBox BoundingBox = StaticMesh->GetBoundingBox();
	FVector MeshCenter = BoundingBox.GetCenter();
	FVector MeshSize = BoundingBox.GetSize();
	
	// Calculate the pivot offset from the mesh center
	// For static meshes, the pivot is typically at the origin of the mesh's local space
	// We need to check if the pivot is significantly offset from the mesh's geometric center
	FVector PivotOffset = FVector::ZeroVector; // Static mesh pivot is at origin
	
	// Calculate the distance from the mesh center to the pivot (origin)
	float DistanceFromCenter = FVector::Dist(MeshCenter, PivotOffset);
	
	// Check if the pivot is too far from the mesh center using the provided thresholds
	// Use the smaller of the two thresholds for detection (WarningDistance)
	float Threshold = FMath::Min(WarningDistance, ErrorDistance);
	
	OutPivotOffset = PivotOffset;
	
	// Flag as problematic if the pivot is significantly offset from the mesh center
	return DistanceFromCenter > Threshold;
}

bool FStaticMeshTransformRule::HasUnappliedDCCTransformations(const UStaticMesh* StaticMesh, TArray<FString>& OutIssues) const
{
	if (!StaticMesh)
	{
		return false;
	}

	// Check for various DCC transformation issues
	FBox BoundingBox = StaticMesh->GetBoundingBox();
	FVector MeshSize = BoundingBox.GetSize();
	FVector MeshCenter = BoundingBox.GetCenter();
	
	bool HasIssues = false;
	
	// Check for pivot not at origin (but this might be intentional for certain asset types)
	float DistanceFromOrigin = FVector::Dist(MeshCenter, FVector::ZeroVector);
	if (DistanceFromOrigin > 1000.0f) // Very far from origin
	{
		OutIssues.Add(TEXT("Pivot very far from origin"));
		HasIssues = true;
	}
	
	// Check for extreme size values that might indicate unapplied scale
	if (MeshSize.X > 1000.0f || MeshSize.Y > 1000.0f || MeshSize.Z > 1000.0f)
	{
		OutIssues.Add(TEXT("Extreme size values (>1000 units)"));
		HasIssues = true;
	}
	
	// Check for very small size values
	if (MeshSize.X < 0.001f || MeshSize.Y < 0.001f || MeshSize.Z < 0.001f)
	{
		OutIssues.Add(TEXT("Very small size values (<0.001 units)"));
		HasIssues = true;
	}
	
	return HasIssues;
}

FString FStaticMeshTransformRule::GenerateTransformPivotDescription(const UStaticMesh* StaticMesh, const FVector& PivotOffset, float WarningDistance, float ErrorDistance) const
{
	FBox BoundingBox = StaticMesh->GetBoundingBox();
	FVector MeshCenter = BoundingBox.GetCenter();
	float DistanceFromCenter = FVector::Dist(MeshCenter, PivotOffset);
	
	return FString::Printf(TEXT("Static mesh %s has pivot offset %.2f units from mesh center. This may cause placement and rotation issues. Consider centering the pivot in your DCC tool."), 
		*StaticMesh->GetName(), DistanceFromCenter);
}

FString FStaticMeshTransformRule::GenerateDCCTransformDescription(const UStaticMesh* StaticMesh, const TArray<FString>& Issues) const
{
	FString IssuesText = FString::Join(Issues, TEXT(", "));
	return FString::Printf(TEXT("Static mesh %s has unapplied DCC transformations: %s. These should be applied before importing."), 
		*StaticMesh->GetName(), *IssuesText);
}

bool FStaticMeshTransformRule::CanSafelyFixTransform(const UStaticMesh* StaticMesh) const
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

bool FStaticMeshTransformRule::FixTransformPivot(UStaticMesh* StaticMesh) const
{
	if (!StaticMesh)
	{
		return false;
	}

	UE_LOG(LogPipelineGuardian, Warning, TEXT("Auto-fix for transform pivot not implemented for %s. Please fix manually in your DCC tool."), *StaticMesh->GetName());
	
	// Mark mesh as dirty to ensure it gets saved
	StaticMesh->MarkPackageDirty();
	
	return false; // Return false since we can't actually fix it automatically
} 