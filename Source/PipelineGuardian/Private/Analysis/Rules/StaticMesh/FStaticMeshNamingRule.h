// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Analysis/IAssetCheckRule.h"

// Forward Declarations
class UStaticMesh;
class UPipelineGuardianProfile;
struct FAssetAnalysisResult;

/**
 * Rule to check Static Mesh naming conventions.
 * Validates that static mesh assets follow the configured naming pattern.
 */
class FStaticMeshNamingRule : public IAssetCheckRule
{
public:
	FStaticMeshNamingRule();
	virtual ~FStaticMeshNamingRule() = default;

	// IAssetCheckRule interface
	virtual bool Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults) override;
	virtual FName GetRuleID() const override;
	virtual FText GetRuleDescription() const override;

private:
	/** Check if the asset name matches the expected pattern */
	bool DoesNameMatchPattern(const FString& AssetName, const FString& Pattern) const;
	
	/** Convert a simple pattern with wildcards to a regex pattern */
	FString ConvertPatternToRegex(const FString& Pattern) const;
	
	/** Generate a suggested name based on the current name and pattern */
	FString GenerateSuggestedName(const FString& CurrentName, const FString& Pattern) const;
	
	/** Fix the asset naming by renaming it to the suggested name */
	static void FixAssetNaming(UStaticMesh* StaticMesh, const FString& NewName);
}; 