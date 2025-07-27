// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Templates/SharedPointer.h"
#include "UObject/Class.h" // For UClass

// Forward Declarations
class IAssetAnalyzer;
class UPipelineGuardianSettings;
struct FAssetAnalysisResult;

/**
 * Scans for assets and orchestrates their analysis using registered analyzers.
 */
class FAssetScanner
{
public:
	FAssetScanner();
	~FAssetScanner();

	/**
	 * Finds assets in a given content path.
	 * @param Path The content path to scan (e.g., /Game/Meshes).
	 * @param bRecursive If true, scan will include sub-folders.
	 * @param OutAssetDataList Array to populate with found asset data.
	 */
	void ScanAssetsInPath(const FString& Path, bool bRecursive, TArray<FAssetData>& OutAssetDataList) const;

	/**
	 * Finds assets currently selected in the Content Browser.
	 * @param OutAssetDataList Array to populate with selected asset data.
	 */
	void ScanSelectedAssets(TArray<FAssetData>& OutAssetDataList) const;

	/**
	 * Registers an asset analyzer for a specific UClass type.
	 * @param AssetClass The UClass to associate the analyzer with.
	 * @param Analyzer The analyzer instance.
	 */
	void RegisterAssetAnalyzer(UClass* AssetClass, TSharedPtr<IAssetAnalyzer> Analyzer);

	/**
	 * Analyzes a single asset using the appropriate registered analyzer.
	 * @param AssetData The FAssetData of the asset to analyze.
	 * @param Settings The current pipeline guardian settings.
	 * @param OutResults Array to populate with any issues found.
	 */
	void AnalyzeSingleAsset(const FAssetData& AssetData, const UPipelineGuardianSettings* Settings, TArray<FAssetAnalysisResult>& OutResults);

	/**
	 * Clears all registered asset analyzers.
	 */
	void UnregisterAllAnalyzers();

private:
	TMap<UClass*, TSharedPtr<IAssetAnalyzer>> AssetAnalyzersMap;
}; 