// Copyright Epic Games, Inc. All Rights Reserved.

#include "Analysis/AssetTypeAnalyzers/FStaticMeshAnalyzer.h"
#include "Analysis/IAssetCheckRule.h"
#include "Analysis/FAssetAnalysisResult.h"
#include "Analysis/FPipelineGuardianProfile.h"
#include "Analysis/Rules/StaticMesh/FStaticMeshNamingRule.h"
#include "Analysis/Rules/StaticMesh/FStaticMeshLODMissingRule.h"
#include "Analysis/Rules/StaticMesh/FStaticMeshLODPolyReductionRule.h"
#include "Analysis/Rules/StaticMesh/FStaticMeshLightmapUVMissingRule.h"
#include "Analysis/Rules/StaticMesh/FStaticMeshUVOverlappingRule.h"
#include "Analysis/Rules/StaticMesh/FStaticMeshTriangleCountRule.h"
#include "Analysis/Rules/StaticMesh/FStaticMeshDegenerateFacesRule.h"
#include "Analysis/Rules/StaticMesh/FStaticMeshCollisionMissingRule.h"
#include "Analysis/Rules/StaticMesh/FStaticMeshCollisionComplexityRule.h"
#include "Analysis/Rules/StaticMesh/FStaticMeshNaniteSuitabilityRule.h"
#include "Analysis/Rules/StaticMesh/FStaticMeshMaterialSlotRule.h"
#include "Analysis/Rules/StaticMesh/FStaticMeshVertexColorMissingRule.h"
#include "Analysis/Rules/StaticMesh/FStaticMeshTransformRule.h"
#include "Analysis/Rules/StaticMesh/FStaticMeshScalingRule.h"
#include "Analysis/Rules/StaticMesh/FStaticMeshLightmapResolutionRule.h"
#include "Analysis/Rules/StaticMesh/FStaticMeshSocketNamingRule.h"
#include "Engine/StaticMesh.h"
#include "AssetRegistry/AssetData.h"
#include "PipelineGuardian.h"

#define LOCTEXT_NAMESPACE "FStaticMeshAnalyzer"

FStaticMeshAnalyzer::FStaticMeshAnalyzer()
{
	InitializeRules();
}

void FStaticMeshAnalyzer::InitializeRules()
{
	// Create instances of all static mesh rules
	StaticMeshRules.Add(MakeShared<FStaticMeshNamingRule>());
	StaticMeshRules.Add(MakeShared<FStaticMeshLODMissingRule>());
	StaticMeshRules.Add(MakeShared<FStaticMeshLODPolyReductionRule>());
	StaticMeshRules.Add(MakeShared<FStaticMeshLightmapUVMissingRule>());
	StaticMeshRules.Add(MakeShared<FStaticMeshUVOverlappingRule>());
	StaticMeshRules.Add(MakeShared<FStaticMeshTriangleCountRule>());
	StaticMeshRules.Add(MakeShared<FStaticMeshDegenerateFacesRule>());
	StaticMeshRules.Add(MakeShared<FStaticMeshCollisionMissingRule>());
	StaticMeshRules.Add(MakeShared<FStaticMeshCollisionComplexityRule>());
	StaticMeshRules.Add(MakeShared<FStaticMeshNaniteSuitabilityRule>());
	StaticMeshRules.Add(MakeShared<FStaticMeshMaterialSlotRule>());
	StaticMeshRules.Add(MakeShared<FStaticMeshVertexColorMissingRule>());
	StaticMeshRules.Add(MakeShared<FStaticMeshTransformRule>());
	StaticMeshRules.Add(MakeShared<FStaticMeshScalingRule>());
	StaticMeshRules.Add(MakeShared<FStaticMeshLightmapResolutionRule>());
	StaticMeshRules.Add(MakeShared<FStaticMeshSocketNamingRule>());
	
	// TODO: Add more rules as they are implemented
	
	UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshAnalyzer initialized with %d rules"), StaticMeshRules.Num());
}

void FStaticMeshAnalyzer::AnalyzeAsset(const FAssetData& AssetData, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults)
{
	if (!AssetData.IsValid())
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("FStaticMeshAnalyzer: Invalid AssetData provided"));
		return;
	}

	if (!Profile)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("FStaticMeshAnalyzer: No profile provided for asset: %s"), *AssetData.AssetName.ToString());
		return;
	}

	// Load the static mesh asset
	UStaticMesh* StaticMesh = Cast<UStaticMesh>(AssetData.GetAsset());
	if (!StaticMesh)
	{
		UE_LOG(LogPipelineGuardian, Error, TEXT("FStaticMeshAnalyzer: Failed to load StaticMesh asset: %s"), *AssetData.AssetName.ToString());
		
		// Add a loading failure result
		FAssetAnalysisResult LoadFailureResult;
		LoadFailureResult.Asset = AssetData;
		LoadFailureResult.Severity = EAssetIssueSeverity::Error;
		LoadFailureResult.RuleID = FName(TEXT("SM_AssetLoading"));
		LoadFailureResult.Description = FText::Format(LOCTEXT("StaticMeshLoadFail", "Failed to load StaticMesh asset: {0}"), FText::FromName(AssetData.AssetName));
		LoadFailureResult.FilePath = FText::FromName(AssetData.PackageName);
		OutResults.Add(LoadFailureResult);
		return;
	}

	UE_LOG(LogPipelineGuardian, Verbose, TEXT("FStaticMeshAnalyzer: Analyzing StaticMesh: %s with %d rules"), *AssetData.AssetName.ToString(), StaticMeshRules.Num());

	// Run all static mesh rules
	for (const TSharedPtr<IAssetCheckRule>& Rule : StaticMeshRules)
	{
		if (Rule.IsValid())
		{
			UE_LOG(LogPipelineGuardian, VeryVerbose, TEXT("FStaticMeshAnalyzer: Running rule %s on asset %s"), *Rule->GetRuleID().ToString(), *AssetData.AssetName.ToString());
			Rule->Check(StaticMesh, Profile, OutResults);
		}
		else
		{
			UE_LOG(LogPipelineGuardian, Warning, TEXT("FStaticMeshAnalyzer: Invalid rule found in StaticMeshRules array"));
		}
	}

	UE_LOG(LogPipelineGuardian, Verbose, TEXT("FStaticMeshAnalyzer: Completed analysis of %s. Total issues found so far: %d"), *AssetData.AssetName.ToString(), OutResults.Num());
}

#undef LOCTEXT_NAMESPACE 