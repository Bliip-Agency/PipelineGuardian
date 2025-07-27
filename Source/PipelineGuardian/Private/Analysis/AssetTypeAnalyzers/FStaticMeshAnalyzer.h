// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Analysis/IAssetAnalyzer.h"
#include "Templates/SharedPointer.h"
#include "Containers/Array.h"

// Forward Declarations
class IAssetCheckRule;
class UStaticMesh;
class UPipelineGuardianProfile;
struct FAssetData;
struct FAssetAnalysisResult;

/**
 * Analyzer for Static Mesh assets.
 * Orchestrates various static mesh-specific rules and checks.
 */
class FStaticMeshAnalyzer : public IAssetAnalyzer
{
public:
	FStaticMeshAnalyzer();
	virtual ~FStaticMeshAnalyzer() = default;

	// IAssetAnalyzer interface
	virtual void AnalyzeAsset(const FAssetData& AssetData, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults) override;

private:
	/** Initialize all static mesh rules */
	void InitializeRules();

	/** Array of all static mesh analysis rules */
	TArray<TSharedPtr<IAssetCheckRule>> StaticMeshRules;
}; 