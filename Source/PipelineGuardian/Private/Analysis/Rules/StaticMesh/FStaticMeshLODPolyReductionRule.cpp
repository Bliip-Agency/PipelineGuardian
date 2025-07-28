// Copyright Epic Games, Inc. All Rights Reserved.

#include "FStaticMeshLODPolyReductionRule.h"
#include "Analysis/FAssetAnalysisResult.h"
#include "Analysis/FPipelineGuardianProfile.h"
#include "Engine/StaticMesh.h"
#include "PipelineGuardian.h"
#include "StaticMeshResources.h"
#include "MeshUtilities.h"
#include "IMeshReductionManagerModule.h"
#include "IMeshReductionInterfaces.h"
#include "Modules/ModuleManager.h"
#include "Misc/MessageDialog.h"

#define LOCTEXT_NAMESPACE "FStaticMeshLODPolyReductionRule"

FStaticMeshLODPolyReductionRule::FStaticMeshLODPolyReductionRule()
{
}

bool FStaticMeshLODPolyReductionRule::Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults)
{
	UStaticMesh* StaticMesh = Cast<UStaticMesh>(Asset);
	if (!StaticMesh)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("FStaticMeshLODPolyReductionRule: Asset is not a UStaticMesh"));
		return false;
	}

	if (!Profile)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("FStaticMeshLODPolyReductionRule: No profile provided"));
		return false;
	}

	// Check if this rule is enabled in the profile
	if (!Profile->IsRuleEnabled(GetRuleID()))
	{
		UE_LOG(LogPipelineGuardian, Verbose, TEXT("FStaticMeshLODPolyReductionRule: Rule is disabled in profile"));
		return false;
	}

	// Get settings from profile
	float MinReductionPercentage = FCString::Atof(*Profile->GetRuleParameter(GetRuleID(), TEXT("MinReductionPercentage"), TEXT("30.0")));
	float WarningThreshold = FCString::Atof(*Profile->GetRuleParameter(GetRuleID(), TEXT("WarningThreshold"), TEXT("20.0")));
	float ErrorThreshold = FCString::Atof(*Profile->GetRuleParameter(GetRuleID(), TEXT("ErrorThreshold"), TEXT("10.0")));
	
	// Ensure we have render data
	if (!StaticMesh->GetRenderData() || StaticMesh->GetRenderData()->LODResources.Num() < 2)
	{
		UE_LOG(LogPipelineGuardian, VeryVerbose, TEXT("FStaticMeshLODPolyReductionRule: %s has insufficient LODs for reduction analysis"), 
			*StaticMesh->GetName());
		return false; // Need at least 2 LODs to check reduction
	}

	int32 LODCount = StaticMesh->GetRenderData()->LODResources.Num();
	bool bFoundIssues = false;

	// Enhanced debugging: Log all LOD triangle counts first
	UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshLODPolyReductionRule: Analyzing %s with %d LODs"), 
		*StaticMesh->GetName(), LODCount);
	
	for (int32 i = 0; i < LODCount; ++i)
	{
		int32 TriangleCount = GetLODTriangleCount(StaticMesh, i);
		UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshLODPolyReductionRule: %s LOD%d has %d triangles"), 
			*StaticMesh->GetName(), i, TriangleCount);
	}

	// Collect all problematic LODs for a comprehensive fix
	TArray<int32> ProblematicLODs;
	FString IssueDescription;
	EAssetIssueSeverity WorstSeverity = EAssetIssueSeverity::Info;

	// Check reduction between each consecutive LOD pair
	for (int32 LODIndex = 1; LODIndex < LODCount; ++LODIndex)
	{
		int32 PreviousLODTriangles = GetLODTriangleCount(StaticMesh, LODIndex - 1);
		int32 CurrentLODTriangles = GetLODTriangleCount(StaticMesh, LODIndex);
		
		// Debug logging to understand the triangle counts
		UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshLODPolyReductionRule: %s LOD%d: %d triangles → LOD%d: %d triangles"), 
			*StaticMesh->GetName(), LODIndex - 1, PreviousLODTriangles, LODIndex, CurrentLODTriangles);
		
		if (PreviousLODTriangles == 0 || CurrentLODTriangles == 0)
		{
			UE_LOG(LogPipelineGuardian, Warning, TEXT("FStaticMeshLODPolyReductionRule: %s has LOD with zero triangles (LOD%d: %d, LOD%d: %d) - SKIPPING"), 
				*StaticMesh->GetName(), LODIndex - 1, PreviousLODTriangles, LODIndex, CurrentLODTriangles);
			continue;
		}

		float ReductionPercentage = CalculateReductionPercentage(PreviousLODTriangles, CurrentLODTriangles);
		
		// Debug logging for reduction calculation
		UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshLODPolyReductionRule: %s LOD%d→LOD%d reduction: %.2f%% (Min required: %.2f%%)"), 
			*StaticMesh->GetName(), LODIndex - 1, LODIndex, ReductionPercentage, MinReductionPercentage);
		
		// Check if reduction is insufficient
		if (ReductionPercentage < MinReductionPercentage)
		{
			UE_LOG(LogPipelineGuardian, Warning, TEXT("FStaticMeshLODPolyReductionRule: ISSUE DETECTED - %s LOD%d→LOD%d has insufficient reduction (%.2f%% < %.2f%%)"), 
				*StaticMesh->GetName(), LODIndex - 1, LODIndex, ReductionPercentage, MinReductionPercentage);
				
			// Add to problematic LODs list
			ProblematicLODs.AddUnique(LODIndex);
			bFoundIssues = true;
			
			// Determine severity and track the worst one
			EAssetIssueSeverity CurrentSeverity = GetSeverityForReduction(ReductionPercentage, MinReductionPercentage, WarningThreshold, ErrorThreshold);
			if (CurrentSeverity > WorstSeverity)
			{
				WorstSeverity = CurrentSeverity;
			}
			
			// Build description for this LOD issue
			FString ReductionText;
			if (ReductionPercentage < 0)
			{
				ReductionText = FString::Printf(TEXT("INCREASE of %.1f%%"), FMath::Abs(ReductionPercentage));
			}
			else
			{
				ReductionText = FString::Printf(TEXT("%.1f%% reduction"), ReductionPercentage);
			}
			
			if (!IssueDescription.IsEmpty())
			{
				IssueDescription += TEXT("; ");
			}
			IssueDescription += FString::Printf(TEXT("LOD%d→LOD%d: %s (need %.1f%%)"), 
				LODIndex - 1, LODIndex, *ReductionText, MinReductionPercentage);
			
			UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshLODPolyReductionRule: Added LOD%d to problematic list"), LODIndex);
		}
		else
		{
			UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshLODPolyReductionRule: %s LOD%d→LOD%d has SUFFICIENT reduction (%.1f%% >= %.1f%%)"), 
				*StaticMesh->GetName(), LODIndex - 1, LODIndex, ReductionPercentage, MinReductionPercentage);
		}
	}

	// If we found issues, create a single comprehensive fix action
	if (bFoundIssues && ProblematicLODs.Num() > 0)
	{
		FAssetAnalysisResult Result;
		Result.Asset = FAssetData(StaticMesh);
		Result.Severity = WorstSeverity;
		Result.RuleID = GetRuleID();
		
		// Create comprehensive description
		Result.Description = FText::Format(
			LOCTEXT("StaticMeshLODPolyReductionComprehensive", "Static Mesh '{0}' has insufficient polygon reduction in {1} LOD level(s): {2}. Required: {3}% reduction between consecutive LODs."),
			FText::FromString(StaticMesh->GetName()),
			FText::AsNumber(ProblematicLODs.Num()),
			FText::FromString(IssueDescription),
			FText::AsNumber(FMath::RoundToInt(MinReductionPercentage))
		);
		Result.FilePath = FText::FromString(StaticMesh->GetPackage()->GetName());
		
		// Create comprehensive fix action that handles all problematic LODs
		if (CanFixLODReduction(StaticMesh))
		{
			Result.FixAction.BindLambda([StaticMesh, ProblematicLODs, MinReductionPercentage]()
			{
				FixAllLODReductions(StaticMesh, ProblematicLODs, MinReductionPercentage);
			});
		}
		
		OutResults.Add(Result);
		
		UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshLODPolyReductionRule: Added comprehensive issue for %s covering %d problematic LODs"), 
			*StaticMesh->GetName(), ProblematicLODs.Num());
	}

	UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshLODPolyReductionRule: Analysis complete for %s - Found issues: %s"), 
		*StaticMesh->GetName(), bFoundIssues ? TEXT("YES") : TEXT("NO"));

	return bFoundIssues;
}

FName FStaticMeshLODPolyReductionRule::GetRuleID() const
{
	return FName(TEXT("SM_LODPolyReduction"));
}

FText FStaticMeshLODPolyReductionRule::GetRuleDescription() const
{
	return LOCTEXT("StaticMeshLODPolyReductionRuleDescription", "Validates that Static Mesh LOD levels have sufficient polygon reduction between consecutive levels for optimal performance.");
}

int32 FStaticMeshLODPolyReductionRule::GetLODTriangleCount(UStaticMesh* StaticMesh, int32 LODIndex) const
{
	if (!StaticMesh || !StaticMesh->GetRenderData())
	{
		return 0;
	}
	
	const FStaticMeshLODResourcesArray& LODResources = StaticMesh->GetRenderData()->LODResources;
	if (!LODResources.IsValidIndex(LODIndex))
	{
		return 0;
	}
	
	return LODResources[LODIndex].GetNumTriangles();
}

float FStaticMeshLODPolyReductionRule::CalculateReductionPercentage(int32 HigherLODTriangles, int32 LowerLODTriangles) const
{
	if (HigherLODTriangles == 0)
	{
		return 0.0f;
	}
	
	// Calculate percentage reduction: (Original - Reduced) / Original * 100
	float ReductionRatio = (float)(HigherLODTriangles - LowerLODTriangles) / (float)HigherLODTriangles;
	return ReductionRatio * 100.0f;
}

bool FStaticMeshLODPolyReductionRule::CanFixLODReduction(UStaticMesh* StaticMesh) const
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

EAssetIssueSeverity FStaticMeshLODPolyReductionRule::GetSeverityForReduction(float ActualReduction, float MinReduction, float WarningThreshold, float ErrorThreshold) const
{

	if (ActualReduction < ErrorThreshold)
	{
		return EAssetIssueSeverity::Error;
	}

	else if (ActualReduction < WarningThreshold)
	{
		return EAssetIssueSeverity::Warning;
	}

	else if (ActualReduction < MinReduction)
	{
		return EAssetIssueSeverity::Info;
	}
	
	// This shouldn't happen if the rule is working correctly
	return EAssetIssueSeverity::Warning;
}

void FStaticMeshLODPolyReductionRule::FixLODReduction(UStaticMesh* StaticMesh, int32 ProblematicLODIndex, float TargetReductionPercentage)
{
	if (!StaticMesh)
	{
		UE_LOG(LogPipelineGuardian, Error, TEXT("FStaticMeshLODPolyReductionRule::FixLODReduction: StaticMesh is null"));
		return;
	}
	
	if (ProblematicLODIndex <= 0 || ProblematicLODIndex >= StaticMesh->GetRenderData()->LODResources.Num())
	{
		UE_LOG(LogPipelineGuardian, Error, TEXT("FStaticMeshLODPolyReductionRule::FixLODReduction: Invalid LOD index %d"), ProblematicLODIndex);
		return;
	}
	
	UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshLODPolyReductionRule: Fixing LOD reduction for '%s' LOD%d with target %.1f%% reduction"), 
		*StaticMesh->GetName(), ProblematicLODIndex, TargetReductionPercentage);

	// Get the mesh reduction interface
	IMeshReductionManagerModule& MeshReductionModule = FModuleManager::Get().LoadModuleChecked<IMeshReductionManagerModule>("MeshReductionInterface");
	IMeshReduction* MeshReduction = MeshReductionModule.GetStaticMeshReductionInterface();
	
	if (!MeshReduction)
	{
		UE_LOG(LogPipelineGuardian, Error, TEXT("FStaticMeshLODPolyReductionRule::FixLODReduction: Mesh reduction interface not available"));
		
		FText Message = FText::Format(
			LOCTEXT("LODReductionFixNoInterface", "Cannot fix LOD reduction for '{0}' because mesh reduction interface is not available.\n\nPlease ensure mesh reduction plugins are enabled in your project."),
			FText::FromString(StaticMesh->GetName())
		);
		FMessageDialog::Open(EAppMsgType::Ok, Message, LOCTEXT("LODReductionFixError", "LOD Reduction Fix Error"));
		return;
	}

	// Prepare the static mesh for modification
	StaticMesh->Modify();
	
	// Get triangle counts BEFORE we start modifying
	int32 BaseLODTriangles = StaticMesh->GetRenderData()->LODResources[0].GetNumTriangles();
	int32 PreviousLODTriangles = StaticMesh->GetRenderData()->LODResources[ProblematicLODIndex - 1].GetNumTriangles();
	int32 CurrentLODTriangles = StaticMesh->GetRenderData()->LODResources[ProblematicLODIndex].GetNumTriangles();
	
	// Calculate what the target triangle count should be for this LOD
	// The target is based on the PREVIOUS LOD, not the current problematic LOD
	int32 TargetTriangleCount = FMath::RoundToInt(PreviousLODTriangles * (100.0f - TargetReductionPercentage) / 100.0f);
	
	// Ensure we don't go below a reasonable minimum
	TargetTriangleCount = FMath::Max(TargetTriangleCount, 4); // At least 4 triangles (minimum for a mesh)
	
	// Calculate the target triangle percentage relative to LOD0 (this is what UE uses internally)
	float TargetTrianglePercentage = (float)TargetTriangleCount / (float)BaseLODTriangles;
	
	TargetTrianglePercentage = FMath::Clamp(TargetTrianglePercentage, PipelineGuardianConstants::MIN_LOD_REDUCTION_CLAMP, PipelineGuardianConstants::MAX_LOD_REDUCTION_CLAMP);
	
	UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshLODPolyReductionRule: Target for LOD%d: %d triangles (%.1f%% of LOD0, %.1f%% reduction from LOD%d)"), 
		ProblematicLODIndex, TargetTriangleCount, TargetTrianglePercentage * 100.0f, TargetReductionPercentage, ProblematicLODIndex - 1);
	
	// Update the source model for this LOD with new reduction settings
	if (StaticMesh->GetSourceModels().IsValidIndex(ProblematicLODIndex))
	{
		// Get a mutable reference to the source model
		FStaticMeshSourceModel& SourceModel = const_cast<FStaticMeshSourceModel&>(StaticMesh->GetSourceModels()[ProblematicLODIndex]);
		
		// Configure reduction settings to achieve the target triangle percentage
		FMeshReductionSettings& ReductionSettings = SourceModel.ReductionSettings;
		ReductionSettings.PercentTriangles = TargetTrianglePercentage;
		ReductionSettings.PercentVertices = TargetTrianglePercentage; // Keep vertex percentage in sync
		ReductionSettings.MaxDeviation = 0.0f; // Let the algorithm decide
		ReductionSettings.PixelError = 8.0f; // Good balance for most meshes
		ReductionSettings.WeldingThreshold = 0.0f;
		ReductionSettings.HardAngleThreshold = 80.0f;
		ReductionSettings.BaseLODModel = 0; // Always reduce from LOD0
		ReductionSettings.SilhouetteImportance = EMeshFeatureImportance::Normal;
		ReductionSettings.TextureImportance = EMeshFeatureImportance::Normal;
		ReductionSettings.ShadingImportance = EMeshFeatureImportance::Normal;
		
		UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshLODPolyReductionRule: Updated LOD%d reduction settings to %.1f%% triangles (relative to LOD0)"), 
			ProblematicLODIndex, TargetTrianglePercentage * 100.0f);
	}
	else
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("FStaticMeshLODPolyReductionRule: No source model found for LOD%d, cannot update reduction settings"), 
			ProblematicLODIndex);
	}
	
	// Build the static mesh to apply the new LOD settings
	StaticMesh->Build(false);
	
	// Mark package as dirty
	StaticMesh->MarkPackageDirty();
	
	// Verify the fix worked by checking the NEW triangle counts
	int32 NewTriangleCount = StaticMesh->GetRenderData()->LODResources[ProblematicLODIndex].GetNumTriangles();
	float ActualReductionFromPrevious = ((float)(PreviousLODTriangles - NewTriangleCount) / (float)PreviousLODTriangles) * 100.0f;
	
	// Calculate tolerance based on the mesh complexity
	float ReductionTolerance = 5.0f; // Allow 5% tolerance for reduction percentage
	
	FText Message;
	if (FMath::Abs(ActualReductionFromPrevious - TargetReductionPercentage) <= ReductionTolerance)
	{
		Message = FText::Format(
			LOCTEXT("LODReductionFixSuccess", "Successfully fixed LOD reduction for '{0}' LOD{1}!\n\nOriginal triangles: {2}\nNew triangles: {3}\nReduction achieved: {4}%\nTarget reduction: {5}%\n\nThe mesh now has proper LOD optimization."),
			FText::FromString(StaticMesh->GetName()),
			FText::AsNumber(ProblematicLODIndex),
			FText::AsNumber(CurrentLODTriangles),
			FText::AsNumber(NewTriangleCount),
			FText::AsNumber(FMath::RoundToInt(ActualReductionFromPrevious)),
			FText::AsNumber(FMath::RoundToInt(TargetReductionPercentage))
		);
		UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshLODPolyReductionRule: Successfully fixed LOD reduction for %s LOD%d (%.1f%% reduction achieved vs %.1f%% target)"), 
			*StaticMesh->GetName(), ProblematicLODIndex, ActualReductionFromPrevious, TargetReductionPercentage);
	}
	else
	{
		Message = FText::Format(
			LOCTEXT("LODReductionFixPartial", "LOD reduction fix completed for '{0}' LOD{1}, but achieved different reduction than target.\n\nOriginal triangles: {2}\nNew triangles: {3}\nReduction achieved: {4}%\nTarget reduction: {5}%\n\nThis may be due to mesh complexity or reduction algorithm limitations."),
			FText::FromString(StaticMesh->GetName()),
			FText::AsNumber(ProblematicLODIndex),
			FText::AsNumber(CurrentLODTriangles),
			FText::AsNumber(NewTriangleCount),
			FText::AsNumber(FMath::RoundToInt(ActualReductionFromPrevious)),
			FText::AsNumber(FMath::RoundToInt(TargetReductionPercentage))
		);
		UE_LOG(LogPipelineGuardian, Warning, TEXT("FStaticMeshLODPolyReductionRule: Partial LOD reduction fix for %s LOD%d (%.1f%% vs %.1f%% target)"), 
			*StaticMesh->GetName(), ProblematicLODIndex, ActualReductionFromPrevious, TargetReductionPercentage);
	}
	
	FMessageDialog::Open(EAppMsgType::Ok, Message, LOCTEXT("LODReductionFixComplete", "LOD Reduction Fix Complete"));
}

void FStaticMeshLODPolyReductionRule::FixAllLODReductions(UStaticMesh* StaticMesh, const TArray<int32>& ProblematicLODs, float TargetReductionPercentage)
{
	if (!StaticMesh)
	{
		UE_LOG(LogPipelineGuardian, Error, TEXT("FStaticMeshLODPolyReductionRule::FixAllLODReductions: StaticMesh is null"));
		return;
	}
	
	if (ProblematicLODs.Num() == 0)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("FStaticMeshLODPolyReductionRule::FixAllLODReductions: No problematic LODs provided"));
		return;
	}
	
	UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshLODPolyReductionRule: Fixing ALL LOD reductions for '%s' - %d problematic LODs with target %.1f%% reduction"), 
		*StaticMesh->GetName(), ProblematicLODs.Num(), TargetReductionPercentage);

	// Get the mesh reduction interface
	IMeshReductionManagerModule& MeshReductionModule = FModuleManager::Get().LoadModuleChecked<IMeshReductionManagerModule>("MeshReductionInterface");
	IMeshReduction* MeshReduction = MeshReductionModule.GetStaticMeshReductionInterface();
	
	if (!MeshReduction)
	{
		UE_LOG(LogPipelineGuardian, Error, TEXT("FStaticMeshLODPolyReductionRule::FixAllLODReductions: Mesh reduction interface not available"));
		
		FText Message = FText::Format(
			LOCTEXT("LODReductionFixNoInterface", "Cannot fix LOD reduction for '{0}' because mesh reduction interface is not available.\n\nPlease ensure mesh reduction plugins are enabled in your project."),
			FText::FromString(StaticMesh->GetName())
		);
		FMessageDialog::Open(EAppMsgType::Ok, Message, LOCTEXT("LODReductionFixError", "LOD Reduction Fix Error"));
		return;
	}

	// Prepare the static mesh for modification
	StaticMesh->Modify();
	
	// Get triangle counts BEFORE we start modifying
	int32 BaseLODTriangles = StaticMesh->GetRenderData()->LODResources[0].GetNumTriangles();
	TArray<int32> OriginalTriangleCounts;
	
	int32 TotalLODs = StaticMesh->GetRenderData()->LODResources.Num();
	for (int32 i = 0; i < TotalLODs; ++i)
	{
		OriginalTriangleCounts.Add(StaticMesh->GetRenderData()->LODResources[i].GetNumTriangles());
	}
	
	UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshLODPolyReductionRule: Original triangle counts - LOD0: %d triangles"), BaseLODTriangles);
	
	// Calculate and apply proper reduction settings for ALL LODs (not just problematic ones)
	// This ensures consistent reduction progression
	for (int32 LODIndex = 1; LODIndex < TotalLODs; ++LODIndex)
	{
		// Calculate what this LOD should have based on progressive reduction
		int32 PreviousLODTriangles = (LODIndex == 1) ? BaseLODTriangles : 
			FMath::RoundToInt(BaseLODTriangles * FMath::Pow((100.0f - TargetReductionPercentage) / 100.0f, LODIndex - 1));
		
		// Calculate target triangle count for this LOD
		int32 TargetTriangleCount = FMath::RoundToInt(PreviousLODTriangles * (100.0f - TargetReductionPercentage) / 100.0f);
		TargetTriangleCount = FMath::Max(TargetTriangleCount, 4); // Minimum 4 triangles
		
		// Calculate the target triangle percentage relative to LOD0
		float TargetTrianglePercentage = (float)TargetTriangleCount / (float)BaseLODTriangles;
		TargetTrianglePercentage = FMath::Clamp(TargetTrianglePercentage, 0.01f, 1.0f);
		
		UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshLODPolyReductionRule: LOD%d target: %d triangles (%.1f%% of LOD0, %.1f%% reduction from previous)"), 
			LODIndex, TargetTriangleCount, TargetTrianglePercentage * 100.0f, TargetReductionPercentage);
		
		// Update the source model for this LOD
		if (StaticMesh->GetSourceModels().IsValidIndex(LODIndex))
		{
			// Get a mutable reference to the source model
			FStaticMeshSourceModel& SourceModel = const_cast<FStaticMeshSourceModel&>(StaticMesh->GetSourceModels()[LODIndex]);
			
			// Configure reduction settings to achieve the target triangle percentage
			FMeshReductionSettings& ReductionSettings = SourceModel.ReductionSettings;
			ReductionSettings.PercentTriangles = TargetTrianglePercentage;
			ReductionSettings.PercentVertices = TargetTrianglePercentage;
			ReductionSettings.MaxDeviation = 0.0f;
			ReductionSettings.PixelError = 8.0f;
			ReductionSettings.WeldingThreshold = 0.0f;
			ReductionSettings.HardAngleThreshold = 80.0f;
			ReductionSettings.BaseLODModel = 0; // Always reduce from LOD0
			ReductionSettings.SilhouetteImportance = EMeshFeatureImportance::Normal;
			ReductionSettings.TextureImportance = EMeshFeatureImportance::Normal;
			ReductionSettings.ShadingImportance = EMeshFeatureImportance::Normal;
			
			UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshLODPolyReductionRule: Updated LOD%d reduction settings to %.1f%% triangles (relative to LOD0)"), 
				LODIndex, TargetTrianglePercentage * 100.0f);
		}
		else
		{
			UE_LOG(LogPipelineGuardian, Warning, TEXT("FStaticMeshLODPolyReductionRule: No source model found for LOD%d, cannot update reduction settings"), 
				LODIndex);
		}
	}
	
	// Build the static mesh to apply the new LOD settings
	StaticMesh->Build(false);
	
	// Mark package as dirty
	StaticMesh->MarkPackageDirty();
	
	// Verify the fix worked by checking ALL LOD triangle counts
	TArray<int32> NewTriangleCounts;
	TArray<float> ActualReductions;
	bool bAllFixesSuccessful = true;
	
	for (int32 LODIndex = 1; LODIndex < TotalLODs; ++LODIndex)
	{
		int32 PreviousTriangles = (LODIndex == 1) ? BaseLODTriangles : NewTriangleCounts[LODIndex - 2];
		int32 NewTriangleCount = StaticMesh->GetRenderData()->LODResources[LODIndex].GetNumTriangles();
		NewTriangleCounts.Add(NewTriangleCount);
		
		float ActualReduction = ((float)(PreviousTriangles - NewTriangleCount) / (float)PreviousTriangles) * 100.0f;
		ActualReductions.Add(ActualReduction);
		
		// Check if this LOD meets the target (allow 5% tolerance)
		if (FMath::Abs(ActualReduction - TargetReductionPercentage) > 5.0f)
		{
			bAllFixesSuccessful = false;
		}
		
		UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshLODPolyReductionRule: LOD%d result: %d→%d triangles (%.1f%% reduction, target %.1f%%)"), 
			LODIndex, PreviousTriangles, NewTriangleCount, ActualReduction, TargetReductionPercentage);
	}
	
	// Create comprehensive result message
	FText Message;
	if (bAllFixesSuccessful)
	{
		FString ReductionSummary;
		for (int32 i = 0; i < ActualReductions.Num(); ++i)
		{
			if (i > 0) ReductionSummary += TEXT(", ");
			ReductionSummary += FString::Printf(TEXT("LOD%d: %.1f%%"), i + 1, ActualReductions[i]);
		}
		
		Message = FText::Format(
			LOCTEXT("LODReductionFixAllSuccess", "Successfully fixed ALL LOD reductions for '{0}'!\n\nReductions achieved: {1}\nTarget reduction: {2}%\n\nThe mesh now has proper progressive LOD optimization."),
			FText::FromString(StaticMesh->GetName()),
			FText::FromString(ReductionSummary),
			FText::AsNumber(FMath::RoundToInt(TargetReductionPercentage))
		);
		UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshLODPolyReductionRule: Successfully fixed ALL LOD reductions for %s"), 
			*StaticMesh->GetName());
	}
	else
	{
		FString ReductionSummary;
		for (int32 i = 0; i < ActualReductions.Num(); ++i)
		{
			if (i > 0) ReductionSummary += TEXT(", ");
			ReductionSummary += FString::Printf(TEXT("LOD%d: %.1f%%"), i + 1, ActualReductions[i]);
		}
		
		Message = FText::Format(
			LOCTEXT("LODReductionFixAllPartial", "LOD reduction fix completed for '{0}', but some LODs may not have reached target reduction.\n\nReductions achieved: {1}\nTarget reduction: {2}%\n\nThis may be due to mesh complexity or reduction algorithm limitations."),
			FText::FromString(StaticMesh->GetName()),
			FText::FromString(ReductionSummary),
			FText::AsNumber(FMath::RoundToInt(TargetReductionPercentage))
		);
		UE_LOG(LogPipelineGuardian, Warning, TEXT("FStaticMeshLODPolyReductionRule: Partial LOD reduction fix for %s"), 
			*StaticMesh->GetName());
	}
	
	FMessageDialog::Open(EAppMsgType::Ok, Message, LOCTEXT("LODReductionFixComplete", "LOD Reduction Fix Complete"));
}

#undef LOCTEXT_NAMESPACE 