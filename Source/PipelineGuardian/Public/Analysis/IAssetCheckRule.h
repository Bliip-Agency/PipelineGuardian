// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Array.h"

// Forward Declarations
struct FAssetAnalysisResult;
class UPipelineGuardianProfile;
class UObject;

class IAssetCheckRule
{
public:
	virtual ~IAssetCheckRule() = default;

	/**
	 * Performs the check on the given AssetObject.
	 * @param AssetObject The loaded UObject to check.
	 * @param Profile The current pipeline guardian profile containing rule configurations.
	 * @param OutResults Array to populate with any issues found.
	 * @return True if any issues were found, false otherwise.
	 */
	virtual bool Check(UObject* AssetObject, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults) = 0;

	/**
	 * Gets the unique identifier for this rule.
	 * @return The FName ID of the rule.
	 */
	virtual FName GetRuleID() const = 0;

	/**
	 * Gets a user-friendly description of what this rule checks for.
	 * @return FText describing the rule.
	 */
	virtual FText GetRuleDescription() const = 0;
}; 