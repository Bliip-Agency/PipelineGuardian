// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "AssetRegistry/AssetData.h"
#include "Delegates/DelegateCombinations.h"
#include "FAssetAnalysisResult.generated.h" // Should be last include in the header

UENUM(BlueprintType)
enum class EAssetIssueSeverity : uint8
{
	Critical UMETA(DisplayName = "Critical Error"),
	Error UMETA(DisplayName = "Error"),
	Warning UMETA(DisplayName = "Warning"),
	Info UMETA(DisplayName = "Information")
};

USTRUCT(BlueprintType)
struct FAssetAnalysisResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnalysisResult")
	FAssetData Asset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnalysisResult")
	EAssetIssueSeverity Severity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnalysisResult")
	FName RuleID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnalysisResult")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnalysisResult")
	FText FilePath;

	// Not a UPROPERTY as FSimpleDelegate is not directly exposable to BP in this way
	// and it's for C++ internal use primarily.
	FSimpleDelegate FixAction;

	FAssetAnalysisResult()
		: Severity(EAssetIssueSeverity::Info)
		, RuleID(NAME_None)
	{
	}

	FAssetAnalysisResult(const FAssetData& InAsset, EAssetIssueSeverity InSeverity, FName InRuleID, const FText& InDescription)
		: Asset(InAsset)
		, Severity(InSeverity)
		, RuleID(InRuleID)
		, Description(InDescription)
	{
		if (Asset.IsValid())
		{
			FilePath = FText::FromName(Asset.PackageName);
		}
	}
}; 