// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/FAssetScanner.h"
#include "PipelineGuardian.h" // For LogPipelineGuardian
#include "Analysis/IAssetAnalyzer.h" // For IAssetAnalyzer TSharedPtr, though often included via FAssetScanner.h through forward decls
#include "FPipelineGuardianSettings.h" // For UPipelineGuardianSettings
#include "Analysis/FPipelineGuardianProfile.h" // For UPipelineGuardianProfile
#include "UObject/UObjectGlobals.h" // For GetName()
#include "AssetRegistry/AssetRegistryModule.h" // For ScanAssetsInPath
#include "ContentBrowserModule.h" // For ScanSelectedAssets
#include "IContentBrowserSingleton.h" // For ScanSelectedAssets
#include "Modules/ModuleManager.h" // For FModuleManager

FAssetScanner::FAssetScanner()
{
	// Constructor can be empty for now
}

FAssetScanner::~FAssetScanner()
{
	UnregisterAllAnalyzers();
}

void FAssetScanner::RegisterAssetAnalyzer(UClass* AssetClass, TSharedPtr<IAssetAnalyzer> Analyzer)
{
	if (AssetClass && Analyzer.IsValid())
	{
		AssetAnalyzersMap.Add(AssetClass, Analyzer);
		UE_LOG(LogPipelineGuardian, Log, TEXT("Registered asset analyzer for class: %s"), *AssetClass->GetName());
	}
	else
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("Failed to register asset analyzer: AssetClass or Analyzer is invalid."));
	}
}

void FAssetScanner::AnalyzeSingleAsset(const FAssetData& AssetData, const UPipelineGuardianSettings* Settings, TArray<FAssetAnalysisResult>& OutResults)
{
	if (!AssetData.IsValid())
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("AnalyzeSingleAsset called with invalid AssetData."));
		return;
	}

	// Load the asset to get its actual UClass for accurate analyzer lookup.
	// The IAssetAnalyzer itself will also load the asset, but we need the class here first.
	UObject* AssetObj = AssetData.GetAsset();
	if (!AssetObj)
	{
		UE_LOG(LogPipelineGuardian, Error, TEXT("Failed to load asset: %s. Cannot perform analysis."), *AssetData.AssetName.ToString());
		// Optionally, create an FAssetAnalysisResult indicating loading failure.
		// FAssetAnalysisResult FailureResult;
		// FailureResult.Asset = AssetData;
		// FailureResult.Severity = EAssetIssueSeverity::Error;
		// FailureResult.RuleID = FName(TEXT("AssetLoading"));
		// FailureResult.Description = FText::Format(LOCTEXT("AssetLoadFail", "Failed to load asset: {0}"), FText::FromName(AssetData.AssetName));
		// FailureResult.FilePath = FText::FromName(AssetData.PackageName);
		// OutResults.Add(FailureResult);
		return;
	}

	UClass* AssetClass = AssetObj->GetClass();
	// AssetClass should be valid if AssetObj is valid.

	TSharedPtr<IAssetAnalyzer> FoundAnalyzer = nullptr;
	UClass* CurrentClass = AssetClass;

	// Iterate up the class hierarchy to find a registered analyzer
	while (CurrentClass != nullptr)
	{
		TSharedPtr<IAssetAnalyzer>* AnalyzerPtr = AssetAnalyzersMap.Find(CurrentClass);
		if (AnalyzerPtr && AnalyzerPtr->IsValid())
		{
			FoundAnalyzer = *AnalyzerPtr;
			UE_LOG(LogPipelineGuardian, Verbose, TEXT("Analyzer found for class %s (Asset: %s, Analyzer for: %s)"), 
				*AssetClass->GetName(), *AssetData.AssetName.ToString(), *CurrentClass->GetName());
			break;
		}
		CurrentClass = CurrentClass->GetSuperClass();
	}

	if (FoundAnalyzer.IsValid())
	{
		// Get the active profile from settings
		const UPipelineGuardianProfile* Profile = Settings ? Settings->GetActiveProfile() : nullptr;
		if (Profile)
		{
			UE_LOG(LogPipelineGuardian, Log, TEXT("Analyzer found for asset type: %s (Asset: %s). Running analysis..."), *AssetClass->GetName(), *AssetData.AssetName.ToString());
			FoundAnalyzer->AnalyzeAsset(AssetData, Profile, OutResults); // Pass original AssetData, analyzer handles actual object if needed
		}
		else
		{
			UE_LOG(LogPipelineGuardian, Warning, TEXT("No active profile available for analysis of asset: %s"), *AssetData.AssetName.ToString());
		}
	}
	else
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("No analyzer registered for asset type: %s (or its parent classes) (Asset: %s)"), *AssetClass->GetName(), *AssetData.AssetName.ToString());
	}
}

void FAssetScanner::ScanAssetsInPath(const FString& Path, bool bRecursive, TArray<FAssetData>& OutAssetDataList) const
{
	OutAssetDataList.Empty();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(*Path));
	Filter.bRecursivePaths = bRecursive;
	// Potentially add ClassNames filter later if needed: Filter.ClassNames.Add(UStaticMesh::StaticClass()->GetFName());

	AssetRegistryModule.Get().GetAssets(Filter, OutAssetDataList);

	UE_LOG(LogPipelineGuardian, Log, TEXT("Found %d assets in path: %s (Recursive: %s)"), OutAssetDataList.Num(), *Path, bRecursive ? TEXT("True") : TEXT("False"));
}

void FAssetScanner::ScanSelectedAssets(TArray<FAssetData>& OutAssetDataList) const
{
	OutAssetDataList.Empty();

	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	ContentBrowserModule.Get().GetSelectedAssets(OutAssetDataList);

	UE_LOG(LogPipelineGuardian, Log, TEXT("Found %d selected assets in Content Browser."), OutAssetDataList.Num());
}

void FAssetScanner::UnregisterAllAnalyzers()
{
	if (AssetAnalyzersMap.Num() > 0)
	{
		UE_LOG(LogPipelineGuardian, Log, TEXT("Unregistering all (%d) asset analyzers."), AssetAnalyzersMap.Num());
		AssetAnalyzersMap.Empty();
	}
	else
	{
		UE_LOG(LogPipelineGuardian, Log, TEXT("No asset analyzers to unregister."));
	}
} 