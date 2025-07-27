// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Engine/DataAsset.h"
#include "FPipelineGuardianProfile.generated.h"

/**
 * Represents a single rule configuration within a profile.
 */
USTRUCT(BlueprintType)
struct PIPELINEGUARDIAN_API FPipelineGuardianRuleConfig
{
	GENERATED_BODY()

	/** Unique identifier for the rule */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule Config")
	FName RuleID;

	/** Whether this rule is enabled in the profile */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule Config")
	bool bEnabled;

	/** Rule-specific parameters stored as key-value pairs */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule Config")
	TMap<FString, FString> Parameters;

	FPipelineGuardianRuleConfig()
		: RuleID(NAME_None)
		, bEnabled(true)
	{
	}

	FPipelineGuardianRuleConfig(FName InRuleID, bool bInEnabled = true)
		: RuleID(InRuleID)
		, bEnabled(bInEnabled)
	{
	}
};

/**
 * A profile containing a collection of rule configurations.
 * Can be saved as a DataAsset or exported/imported as JSON.
 */
UCLASS(BlueprintType)
class PIPELINEGUARDIAN_API UPipelineGuardianProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	UPipelineGuardianProfile();

	/** Display name for this profile */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Profile")
	FString ProfileName;

	/** Description of what this profile is intended for */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Profile")
	FString Description;

	/** Version of this profile for compatibility tracking */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Profile")
	int32 Version;

	/** Collection of rule configurations in this profile */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Profile")
	TArray<FPipelineGuardianRuleConfig> RuleConfigs;

	/**
	 * Gets the configuration for a specific rule.
	 * @param RuleID The ID of the rule to find.
	 * @return Copy of the rule config if found, otherwise returns default config with NAME_None.
	 */
	UFUNCTION(BlueprintCallable, Category = "Profile")
	FPipelineGuardianRuleConfig GetRuleConfig(FName RuleID) const;

	/**
	 * Gets the configuration for a specific rule (C++ only).
	 * @param RuleID The ID of the rule to find.
	 * @return Pointer to the rule config if found, nullptr otherwise.
	 */
	const FPipelineGuardianRuleConfig* GetRuleConfigPtr(FName RuleID) const;

	/**
	 * Sets or updates the configuration for a specific rule.
	 * @param RuleConfig The rule configuration to set.
	 */
	UFUNCTION(BlueprintCallable, Category = "Profile")
	void SetRuleConfig(const FPipelineGuardianRuleConfig& RuleConfig);

	/**
	 * Checks if a rule is enabled in this profile.
	 * @param RuleID The ID of the rule to check.
	 * @return True if the rule is enabled, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Profile")
	bool IsRuleEnabled(FName RuleID) const;

	/**
	 * Gets a parameter value for a specific rule.
	 * @param RuleID The ID of the rule.
	 * @param ParameterName The name of the parameter.
	 * @param DefaultValue The default value to return if not found.
	 * @return The parameter value or default value.
	 */
	UFUNCTION(BlueprintCallable, Category = "Profile")
	FString GetRuleParameter(FName RuleID, const FString& ParameterName, const FString& DefaultValue = TEXT("")) const;

	/**
	 * Exports this profile to a JSON string for external editing.
	 * @return JSON representation of the profile.
	 */
	UFUNCTION(BlueprintCallable, Category = "Profile")
	FString ExportToJSON() const;

	/**
	 * Imports profile data from a JSON string.
	 * @param JSONString The JSON data to import.
	 * @return True if import was successful, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Profile")
	bool ImportFromJSON(const FString& JSONString);

private:
	/** Initializes the profile with default rule configurations */
	void InitializeDefaultRules();
}; 