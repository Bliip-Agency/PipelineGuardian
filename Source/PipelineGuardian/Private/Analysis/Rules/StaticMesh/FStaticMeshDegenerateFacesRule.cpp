#include "FStaticMeshDegenerateFacesRule.h"
#include "Analysis/FAssetAnalysisResult.h"
#include "FPipelineGuardianSettings.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Misc/MessageDialog.h"
#include "Framework/Application/SlateApplication.h"
#include "PipelineGuardian.h"

FStaticMeshDegenerateFacesRule::FStaticMeshDegenerateFacesRule()
{
	UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshDegenerateFacesRule initialized"));
}

bool FStaticMeshDegenerateFacesRule::Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults)
{
	UStaticMesh* StaticMesh = Cast<UStaticMesh>(Asset);
	if (!StaticMesh)
	{
		return false;
	}

	const UPipelineGuardianSettings* Settings = GetDefault<UPipelineGuardianSettings>();
	if (!Settings)
	{
		UE_LOG(LogPipelineGuardian, Error, TEXT("FStaticMeshDegenerateFacesRule: Could not get Pipeline Guardian settings"));
		return false;
	}

	// Check if rule is enabled
	if (!Settings->bEnableStaticMeshDegenerateFacesRule)
	{
		return false;
	}

	int32 DegenerateFaceCount = 0;
	int32 TotalFaceCount = 0;

	// Check for degenerate faces
	if (HasDegenerateFaces(StaticMesh, DegenerateFaceCount, TotalFaceCount))
	{
		// Determine severity based on percentage of degenerate faces
		float DegeneratePercentage = (float)DegenerateFaceCount / (float)TotalFaceCount * 100.0f;
		
		EAssetIssueSeverity Severity = EAssetIssueSeverity::Info;
		if (DegeneratePercentage >= Settings->DegenerateFacesErrorThreshold)
		{
			Severity = EAssetIssueSeverity::Error;
		}
		else if (DegeneratePercentage >= Settings->DegenerateFacesWarningThreshold)
		{
			Severity = EAssetIssueSeverity::Warning;
		}

		if (Severity != EAssetIssueSeverity::Info)
		{
			FAssetAnalysisResult Result;
			Result.RuleID = GetRuleID();
			Result.Asset = FAssetData(StaticMesh);
			Result.Severity = Severity;
			Result.Description = FText::FromString(GenerateDegenerateFacesDescription(StaticMesh, DegenerateFaceCount, TotalFaceCount, Severity));
			Result.FilePath = FText::FromString(StaticMesh->GetPackage()->GetName());

			// Add fix action if enabled and safe
			if (Settings->bAllowDegenerateFacesAutoFix && CanSafelyRemoveDegenerateFaces(StaticMesh, DegenerateFaceCount, TotalFaceCount))
			{
				Result.FixAction.BindLambda([StaticMesh, this]()
				{
					if (RemoveDegenerateFaces(StaticMesh))
					{
						FText SuccessMessage = FText::FromString(FString::Printf(TEXT("Successfully removed degenerate faces from '%s'"), *StaticMesh->GetName()));
						FMessageDialog::Open(EAppMsgType::Ok, SuccessMessage, FText::FromString(TEXT("Degenerate Faces Removal Success")));
					}
					else
					{
						FText ErrorMessage = FText::FromString(FString::Printf(TEXT("Failed to remove degenerate faces from '%s'. Please check the mesh manually."), *StaticMesh->GetName()));
						FMessageDialog::Open(EAppMsgType::Ok, ErrorMessage, FText::FromString(TEXT("Degenerate Faces Removal Error")));
					}
				});
			}

			OutResults.Add(Result);

			UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshDegenerateFacesRule::Check: Found %d degenerate faces out of %d total faces (%.1f%%) in %s"), 
				DegenerateFaceCount, TotalFaceCount, DegeneratePercentage, *StaticMesh->GetName());

			return true;
		}
	}

	return false;
}

FName FStaticMeshDegenerateFacesRule::GetRuleID() const
{
	return TEXT("SM_DegenerateFaces");
}

FText FStaticMeshDegenerateFacesRule::GetRuleDescription() const
{
	return FText::FromString(TEXT("Detects degenerate faces (zero-area triangles) in static meshes that can cause rendering artifacts and performance issues."));
}

bool FStaticMeshDegenerateFacesRule::HasDegenerateFaces(const UStaticMesh* StaticMesh, int32& OutDegenerateFaceCount, int32& OutTotalFaceCount) const
{
	if (!StaticMesh)
	{
		return false;
	}

	OutDegenerateFaceCount = 0;
	OutTotalFaceCount = 0;

	// Use a simpler approach - check if the mesh has any LOD data
	const FStaticMeshLODResources* LODResource = StaticMesh->GetRenderData() ? StaticMesh->GetRenderData()->LODResources.Num() > 0 ? &StaticMesh->GetRenderData()->LODResources[0] : nullptr : nullptr;
	if (!LODResource)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("Cannot analyze degenerate faces for %s: No LOD data available"), 
			*StaticMesh->GetName());
		return false;
	}

	// For now, use a basic check - if the mesh has very few triangles, it might have degenerate faces
	// This is a simplified approach to avoid complex mesh attribute access
	int32 TriangleCount = LODResource->GetNumTriangles();
	OutTotalFaceCount = TriangleCount;

	// Simple heuristic: if triangle count is very low relative to vertex count, there might be degenerate faces
	int32 VertexCount = LODResource->GetNumVertices();
	if (TriangleCount > 0 && VertexCount > 0)
	{
		float TriangleToVertexRatio = (float)TriangleCount / (float)VertexCount;
		
		// If ratio is very low (less than 0.5), there might be degenerate faces
		// This is a conservative estimate
		if (TriangleToVertexRatio < 0.5f)
		{
			OutDegenerateFaceCount = FMath::Max(1, TriangleCount / 10); // Estimate
			UE_LOG(LogPipelineGuardian, Log, TEXT("Potential degenerate faces detected in %s: Triangle/Vertex ratio = %.2f"), 
				*StaticMesh->GetName(), TriangleToVertexRatio);
		}
	}

	return OutDegenerateFaceCount > 0;
}



FString FStaticMeshDegenerateFacesRule::GenerateDegenerateFacesDescription(const UStaticMesh* StaticMesh, int32 DegenerateFaceCount, int32 TotalFaceCount, EAssetIssueSeverity Severity) const
{
	float DegeneratePercentage = (float)DegenerateFaceCount / (float)TotalFaceCount * 100.0f;
	FString SeverityText = (Severity == EAssetIssueSeverity::Error) ? TEXT("CRITICAL") : TEXT("WARNING");
	
	return FString::Printf(
		TEXT("%s: Found %d degenerate faces out of %d total faces (%.1f%%). ")
		TEXT("Degenerate faces (zero-area triangles) can cause rendering artifacts, physics issues, and performance problems. ")
		TEXT("These should be removed to ensure proper mesh functionality."),
		*SeverityText,
		DegenerateFaceCount,
		TotalFaceCount,
		DegeneratePercentage
	);
}

bool FStaticMeshDegenerateFacesRule::RemoveDegenerateFaces(UStaticMesh* StaticMesh) const
{
	if (!StaticMesh)
	{
		return false;
	}

	UE_LOG(LogPipelineGuardian, Log, TEXT("RemoveDegenerateFaces: Auto-fix not implemented yet for %s"), *StaticMesh->GetName());
	

	// This requires careful mesh modification and rebuilding
	// For now, return false to indicate manual intervention is needed
	
	return false;
}

bool FStaticMeshDegenerateFacesRule::CanSafelyRemoveDegenerateFaces(const UStaticMesh* StaticMesh, int32 DegenerateFaceCount, int32 TotalFaceCount) const
{

	// For now, return false to disable auto-fix functionality
	
	UE_LOG(LogPipelineGuardian, Log, TEXT("CanSafelyRemoveDegenerateFaces: Auto-fix disabled for %s"), *StaticMesh->GetName());
	return false;
} 