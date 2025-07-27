// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h" // For FAssetData parameter
#include "Containers/Array.h"      // For TArray

// Forward Declarations
struct FAssetAnalysisResult;
class UPipelineGuardianProfile;

/**
 * Interface for an asset analyzer, responsible for loading an asset (if needed)
 * and orchestrating various checks (IAssetCheckRule instances) on it.
 */
class IAssetAnalyzer
{
public:
	virtual ~IAssetAnalyzer() = default;

	/**
	 * Analyzes a given asset using a set of rules.
	 * @param AssetData The FAssetData of the asset to analyze.
	 * @param Profile The current pipeline guardian profile containing rule configurations.
	 * @param OutResults Array to populate with any issues found.
	 */
	virtual void AnalyzeAsset(const FAssetData& AssetData, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults) = 0;
}; 