// Copyright Epic Games, Inc. All Rights Reserved.

#include "FStaticMeshLODMissingRule.h"
#include "Analysis/FAssetAnalysisResult.h"
#include "Analysis/FPipelineGuardianProfile.h"
#include "FPipelineGuardianSettings.h"
#include "Engine/StaticMesh.h"
#include "PipelineGuardian.h"
#include "StaticMeshResources.h"
#include "MeshUtilities.h"
#include "IMeshReductionManagerModule.h"
#include "IMeshReductionInterfaces.h"
#include "Modules/ModuleManager.h"
#include "Misc/MessageDialog.h"
#include "Engine/StaticMeshSourceData.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"

#define LOCTEXT_NAMESPACE "FStaticMeshLODMissingRule"

FStaticMeshLODMissingRule::FStaticMeshLODMissingRule()
{
}

bool FStaticMeshLODMissingRule::Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults)
{
	UStaticMesh* StaticMesh = Cast<UStaticMesh>(Asset);
	if (!StaticMesh)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("FStaticMeshLODMissingRule: Asset is not a UStaticMesh"));
		return false;
	}

	if (!Profile)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("FStaticMeshLODMissingRule: No profile provided"));
		return false;
	}

	// Check if this rule is enabled in the profile
	if (!Profile->IsRuleEnabled(GetRuleID()))
	{
		UE_LOG(LogPipelineGuardian, Verbose, TEXT("FStaticMeshLODMissingRule: Rule is disabled in profile"));
		return false;
	}

	// Get the minimum required LODs from the profile
	int32 MinRequiredLODs = FCString::Atoi(*Profile->GetRuleParameter(GetRuleID(), TEXT("MinLODs_SM"), TEXT("3")));
	
	// Get current LOD count
	int32 CurrentLODCount = GetStaticMeshLODCount(StaticMesh);
	
	// Check if LODs are missing
	if (CurrentLODCount < MinRequiredLODs)
	{
		// Determine severity based on how many LODs are missing
		EAssetIssueSeverity Severity = EAssetIssueSeverity::Warning;
		if (CurrentLODCount == 1)
		{
			Severity = EAssetIssueSeverity::Error; // Only base LOD is critical
		}
		else if (CurrentLODCount < (MinRequiredLODs / 2))
		{
			Severity = EAssetIssueSeverity::Error; // Less than half required LODs
		}
		
		FAssetAnalysisResult Result;
		Result.Asset = FAssetData(StaticMesh);
		Result.Severity = Severity;
		Result.RuleID = GetRuleID();
		Result.Description = FText::Format(
			LOCTEXT("StaticMeshLODMissing", "Static Mesh '{0}' has {1} LOD(s) but requires {2} LOD(s) for proper optimization"),
			FText::FromString(StaticMesh->GetName()),
			FText::AsNumber(CurrentLODCount),
			FText::AsNumber(MinRequiredLODs)
		);
		Result.FilePath = FText::FromString(StaticMesh->GetPackage()->GetName());
		
		// Create fix action if LOD generation is possible
		if (CanGenerateLODs(StaticMesh))
		{
			Result.FixAction.BindLambda([StaticMesh, MinRequiredLODs]()
			{
				GenerateLODs(StaticMesh, MinRequiredLODs);
			});
		}
		
		OutResults.Add(Result);
		
		UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshLODMissingRule: LOD deficiency found for %s (%d/%d LODs)"), 
			*StaticMesh->GetName(), CurrentLODCount, MinRequiredLODs);
		return true; // Issue found
	}
	else
	{
		UE_LOG(LogPipelineGuardian, VeryVerbose, TEXT("FStaticMeshLODMissingRule: %s has sufficient LODs (%d/%d)"), 
			*StaticMesh->GetName(), CurrentLODCount, MinRequiredLODs);
		return false; // No issues found
	}
}

FName FStaticMeshLODMissingRule::GetRuleID() const
{
	return FName(TEXT("SM_LODMissing"));
}

FText FStaticMeshLODMissingRule::GetRuleDescription() const
{
	return LOCTEXT("StaticMeshLODMissingRuleDescription", "Validates that Static Mesh assets have the minimum required number of LOD levels for performance optimization.");
}

int32 FStaticMeshLODMissingRule::GetStaticMeshLODCount(UStaticMesh* StaticMesh) const
{
	if (!StaticMesh || !StaticMesh->GetRenderData())
	{
		return 0;
	}
	
	return StaticMesh->GetRenderData()->LODResources.Num();
}

bool FStaticMeshLODMissingRule::CanGenerateLODs(UStaticMesh* StaticMesh) const
{
	if (!StaticMesh)
	{
		return false;
	}
	
	// Check if mesh has valid geometry
	if (!StaticMesh->GetRenderData() || StaticMesh->GetRenderData()->LODResources.Num() == 0)
	{
		return false;
	}
	
	// Check if base LOD has vertices
	const FStaticMeshLODResources& BaseLOD = StaticMesh->GetRenderData()->LODResources[0];
	if (BaseLOD.GetNumVertices() == 0)
	{
		return false;
	}
	
	// Check if mesh reduction is available
	IMeshReductionManagerModule& MeshReductionModule = FModuleManager::Get().LoadModuleChecked<IMeshReductionManagerModule>("MeshReductionInterface");
	IMeshReduction* MeshReduction = MeshReductionModule.GetStaticMeshReductionInterface();
	
	return MeshReduction != nullptr;
}

void FStaticMeshLODMissingRule::GenerateLODs(UStaticMesh* StaticMesh, int32 TargetLODCount)
{
	if (!StaticMesh)
	{
		UE_LOG(LogPipelineGuardian, Error, TEXT("FStaticMeshLODMissingRule::GenerateLODs: StaticMesh is null"));
		return;
	}
	
	int32 CurrentLODCount = StaticMesh->GetRenderData()->LODResources.Num();
	if (CurrentLODCount >= TargetLODCount)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("FStaticMeshLODMissingRule::GenerateLODs: Mesh already has sufficient LODs"));
		return;
	}
	
	// Get settings to determine if we should follow LOD quality settings
	const UPipelineGuardianSettings* Settings = GetDefault<UPipelineGuardianSettings>();
	
	// Use standard UE 5.5 mesh reduction interface
	UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshLODMissingRule: Generating LODs for '%s' using %s"), 
		*StaticMesh->GetName(), 
		Settings->bFollowLODQualitySettingsWhenCreating ? TEXT("LOD quality settings") : TEXT("standard mesh reduction"));

	// Get the mesh reduction interface
	IMeshReductionManagerModule& MeshReductionModule = FModuleManager::Get().LoadModuleChecked<IMeshReductionManagerModule>("MeshReductionInterface");
	IMeshReduction* MeshReduction = MeshReductionModule.GetStaticMeshReductionInterface();
	
	if (!MeshReduction)
	{
		UE_LOG(LogPipelineGuardian, Error, TEXT("FStaticMeshLODMissingRule::GenerateLODs: Mesh reduction interface not available"));
		
		FText Message = FText::Format(
			LOCTEXT("LODGenerationNoInterface", "Cannot generate LODs for '{0}' because mesh reduction interface is not available.\n\nPlease ensure mesh reduction plugins are enabled in your project."),
			FText::FromString(StaticMesh->GetName())
		);
		FMessageDialog::Open(EAppMsgType::Ok, Message, LOCTEXT("LODGenerationError", "LOD Generation Error"));
		return;
	}

	// Prepare the static mesh for modification
	StaticMesh->Modify();
	
	// Generate LODs progressively
	int32 LODsToGenerate = FMath::Min(TargetLODCount - CurrentLODCount, 3); // Limit to 3 additional LODs
	bool bGeneratedAnyLODs = false;
	
	// Get base LOD triangle count for percentage calculations
	int32 BaseLODTriangles = StaticMesh->GetRenderData()->LODResources[0].GetNumTriangles();
	
	for (int32 LODIndex = CurrentLODCount; LODIndex < CurrentLODCount + LODsToGenerate; ++LODIndex)
	{
		float TargetTrianglePercentage;
		
		if (Settings->bFollowLODQualitySettingsWhenCreating)
		{
			// Calculate target based on LOD quality settings
			float ReductionFromPrevious;
			
			// Use custom reduction percentages if available, otherwise use MinLODReductionPercentage
			if (Settings->DefaultLODReductionPercentages.IsValidIndex(LODIndex - 1))
			{
				ReductionFromPrevious = Settings->DefaultLODReductionPercentages[LODIndex - 1];
			}
			else
			{
				ReductionFromPrevious = Settings->MinLODReductionPercentage;
			}
			
			// Calculate what the previous LOD should have (or actually has)
			int32 PreviousLODTriangles;
			if (LODIndex - 1 < StaticMesh->GetRenderData()->LODResources.Num())
			{
				// Previous LOD exists, use its actual triangle count
				PreviousLODTriangles = StaticMesh->GetRenderData()->LODResources[LODIndex - 1].GetNumTriangles();
			}
			else
			{
				// Previous LOD doesn't exist yet, calculate what it should be
				// This shouldn't happen in normal flow, but handle it gracefully
				PreviousLODTriangles = BaseLODTriangles;
				for (int32 i = 1; i < LODIndex; ++i)
				{
					float PrevReduction = Settings->DefaultLODReductionPercentages.IsValidIndex(i - 1) ? 
						Settings->DefaultLODReductionPercentages[i - 1] : Settings->MinLODReductionPercentage;
					PreviousLODTriangles = FMath::RoundToInt(PreviousLODTriangles * (100.0f - PrevReduction) / 100.0f);
				}
			}
			
			// Calculate target triangle count for this LOD
			int32 TargetTriangleCount = FMath::RoundToInt(PreviousLODTriangles * (100.0f - ReductionFromPrevious) / 100.0f);
			TargetTriangleCount = FMath::Max(TargetTriangleCount, 4); // Minimum 4 triangles
			
			// Convert to percentage of base LOD
			TargetTrianglePercentage = (float)TargetTriangleCount / (float)BaseLODTriangles;
			
			UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshLODMissingRule: LOD%d target: %.1f%% reduction from LOD%d (%d→%d triangles, %.1f%% of LOD0)"), 
				LODIndex, ReductionFromPrevious, LODIndex - 1, PreviousLODTriangles, TargetTriangleCount, TargetTrianglePercentage * 100.0f);
		}
		else
		{
			// Use standard progressive reduction (50% reduction per LOD level)
			TargetTrianglePercentage = FMath::Pow(0.5f, LODIndex);
			
			UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshLODMissingRule: LOD%d using standard reduction: %.1f%% of LOD0"), 
				LODIndex, TargetTrianglePercentage * 100.0f);
		}
		
		// Clamp to reasonable bounds
		TargetTrianglePercentage = FMath::Clamp(TargetTrianglePercentage, PipelineGuardianConstants::MIN_LOD_REDUCTION_CLAMP, PipelineGuardianConstants::MAX_LOD_REDUCTION_CLAMP);
		
		// Create reduction settings
		FMeshReductionSettings ReductionSettings;
		ReductionSettings.PercentTriangles = TargetTrianglePercentage;
		ReductionSettings.PercentVertices = TargetTrianglePercentage;
		ReductionSettings.MaxDeviation = 0.0f; // Let the algorithm decide
		ReductionSettings.PixelError = 8.0f; // Good balance for most meshes
		ReductionSettings.WeldingThreshold = 0.0f;
		ReductionSettings.HardAngleThreshold = 80.0f;
		ReductionSettings.BaseLODModel = 0; // Always reduce from LOD0
		ReductionSettings.SilhouetteImportance = EMeshFeatureImportance::Normal;
		ReductionSettings.TextureImportance = EMeshFeatureImportance::Normal;
		ReductionSettings.ShadingImportance = EMeshFeatureImportance::Normal;
		
		// Add a new source model for this LOD
		FStaticMeshSourceModel& SourceModel = StaticMesh->AddSourceModel();
		SourceModel.ReductionSettings = ReductionSettings;
		
		UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshLODMissingRule: Added LOD%d with %.1f%% triangles (relative to LOD0)"), 
			LODIndex, TargetTrianglePercentage * 100.0f);
		
		bGeneratedAnyLODs = true;
	}
	
	if (bGeneratedAnyLODs)
	{
		// Build the static mesh to apply the new LODs
		StaticMesh->Build(false);
		
		// Mark package as dirty
		StaticMesh->MarkPackageDirty();
		
		// Verify LODs were created
		int32 NewLODCount = StaticMesh->GetRenderData()->LODResources.Num();
		
		FText Message;
		if (NewLODCount > CurrentLODCount)
		{
			FString MethodUsed = Settings->bFollowLODQualitySettingsWhenCreating ? 
				TEXT("LOD quality settings") : TEXT("standard progressive reduction");
				
			Message = FText::Format(
				LOCTEXT("LODGenerationSuccess", "Successfully generated LODs for '{0}' using {1}!\n\nPrevious LODs: {2}\nNew LODs: {3}\n\nThe mesh now has improved performance optimization."),
				FText::FromString(StaticMesh->GetName()),
				FText::FromString(MethodUsed),
				FText::AsNumber(CurrentLODCount),
				FText::AsNumber(NewLODCount)
			);
			UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshLODMissingRule: Successfully generated LODs for %s (%d -> %d LODs) using %s"), 
				*StaticMesh->GetName(), CurrentLODCount, NewLODCount, *MethodUsed);
		}
		else
		{
			Message = FText::Format(
				LOCTEXT("LODGenerationPartial", "LOD generation completed for '{0}', but may not have reached target count.\n\nCurrent LODs: {1}\nTarget LODs: {2}\n\nThe mesh may not be suitable for further reduction."),
				FText::FromString(StaticMesh->GetName()),
				FText::AsNumber(NewLODCount),
				FText::AsNumber(TargetLODCount)
			);
			UE_LOG(LogPipelineGuardian, Warning, TEXT("FStaticMeshLODMissingRule: Partial LOD generation for %s (%d LODs, target was %d)"), 
				*StaticMesh->GetName(), NewLODCount, TargetLODCount);
		}
		
		FMessageDialog::Open(EAppMsgType::Ok, Message, LOCTEXT("LODGenerationComplete", "LOD Generation Complete"));
	}
}

#undef LOCTEXT_NAMESPACE 