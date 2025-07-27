// Copyright Epic Games, Inc. All Rights Reserved.

#include "Analysis/Rules/StaticMesh/FStaticMeshNamingRule.h"
#include "Analysis/FAssetAnalysisResult.h"
#include "Analysis/FPipelineGuardianProfile.h"
#include "Engine/StaticMesh.h"
#include "PipelineGuardian.h"
#include "Internationalization/Regex.h"
#include "Subsystems/EditorAssetSubsystem.h"
#include "Editor.h"
#include "Misc/MessageDialog.h"
#include "Misc/PackageName.h"

#define LOCTEXT_NAMESPACE "FStaticMeshNamingRule"

FStaticMeshNamingRule::FStaticMeshNamingRule()
{
}

bool FStaticMeshNamingRule::Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults)
{
	UStaticMesh* StaticMesh = Cast<UStaticMesh>(Asset);
	if (!StaticMesh)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("FStaticMeshNamingRule: Asset is not a UStaticMesh"));
		return false;
	}

	if (!Profile)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("FStaticMeshNamingRule: No profile provided"));
		return false;
	}

	// Check if this rule is enabled in the profile
	if (!Profile->IsRuleEnabled(GetRuleID()))
	{
		UE_LOG(LogPipelineGuardian, Verbose, TEXT("FStaticMeshNamingRule: Rule is disabled in profile"));
		return false;
	}

	// Get the naming pattern from the profile
	FString NamingPattern = Profile->GetRuleParameter(GetRuleID(), TEXT("NamingPattern"), TEXT("SM_*"));
	
	// Get the asset name
	FString AssetName = StaticMesh->GetName();
	
	// Check if the name matches the pattern
	if (!DoesNameMatchPattern(AssetName, NamingPattern))
	{
		FAssetAnalysisResult Result;
		Result.Asset = FAssetData(StaticMesh);
		Result.Severity = EAssetIssueSeverity::Warning;
		Result.RuleID = GetRuleID();
		Result.Description = FText::Format(
			LOCTEXT("StaticMeshNamingViolation", "Static Mesh '{0}' does not follow the naming convention. Expected pattern: '{1}'"),
			FText::FromString(AssetName),
			FText::FromString(NamingPattern)
		);
		Result.FilePath = FText::FromString(StaticMesh->GetPackage()->GetName());
		
		// Create fix action
		FString SuggestedName = GenerateSuggestedName(AssetName, NamingPattern);
		Result.FixAction.BindLambda([StaticMesh, SuggestedName]()
		{
			FixAssetNaming(StaticMesh, SuggestedName);
		});
		
		OutResults.Add(Result);
		
		UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshNamingRule: Naming violation found for %s (expected pattern: %s)"), 
			*AssetName, *NamingPattern);
		return true; // Issue found
	}
	else
	{
		UE_LOG(LogPipelineGuardian, VeryVerbose, TEXT("FStaticMeshNamingRule: %s passes naming convention check"), *AssetName);
		return false; // No issues found
	}
}

FName FStaticMeshNamingRule::GetRuleID() const
{
	return FName(TEXT("SM_Naming"));
}

FText FStaticMeshNamingRule::GetRuleDescription() const
{
	return LOCTEXT("StaticMeshNamingRuleDescription", "Validates that Static Mesh assets follow the configured naming convention pattern.");
}

bool FStaticMeshNamingRule::DoesNameMatchPattern(const FString& AssetName, const FString& Pattern) const
{
	// Convert simple pattern to regex
	FString RegexPattern = ConvertPatternToRegex(Pattern);
	
	// Create and test regex
	FRegexPattern CompiledPattern(RegexPattern);
	FRegexMatcher Matcher(CompiledPattern, AssetName);
	
	return Matcher.FindNext();
}

FString FStaticMeshNamingRule::ConvertPatternToRegex(const FString& Pattern) const
{
	FString RegexPattern = Pattern;
	
	// Escape special regex characters except * and ?
	RegexPattern = RegexPattern.Replace(TEXT("."), TEXT("\\."));
	RegexPattern = RegexPattern.Replace(TEXT("+"), TEXT("\\+"));
	RegexPattern = RegexPattern.Replace(TEXT("^"), TEXT("\\^"));
	RegexPattern = RegexPattern.Replace(TEXT("$"), TEXT("\\$"));
	RegexPattern = RegexPattern.Replace(TEXT("("), TEXT("\\("));
	RegexPattern = RegexPattern.Replace(TEXT(")"), TEXT("\\)"));
	RegexPattern = RegexPattern.Replace(TEXT("["), TEXT("\\["));
	RegexPattern = RegexPattern.Replace(TEXT("]"), TEXT("\\]"));
	RegexPattern = RegexPattern.Replace(TEXT("{"), TEXT("\\{"));
	RegexPattern = RegexPattern.Replace(TEXT("}"), TEXT("\\}"));
	RegexPattern = RegexPattern.Replace(TEXT("|"), TEXT("\\|"));
	
	// Convert wildcards to regex equivalents
	RegexPattern = RegexPattern.Replace(TEXT("*"), TEXT(".*"));  // * becomes .*
	RegexPattern = RegexPattern.Replace(TEXT("?"), TEXT("."));   // ? becomes .
	
	// Anchor the pattern to match the entire string
	RegexPattern = TEXT("^") + RegexPattern + TEXT("$");
	
	return RegexPattern;
}

FString FStaticMeshNamingRule::GenerateSuggestedName(const FString& CurrentName, const FString& Pattern) const
{
	// For simple patterns like "SM_*", replace * with the current name
	if (Pattern.Contains(TEXT("*")))
	{
		FString SuggestedName = Pattern;
		
		// If the current name already starts with the prefix, don't duplicate it
		FString Prefix = Pattern.Left(Pattern.Find(TEXT("*")));
		if (CurrentName.StartsWith(Prefix))
		{
			return CurrentName; // Already has correct prefix, shouldn't happen if we got here
		}
		
		// Replace * with the current name
		SuggestedName = SuggestedName.Replace(TEXT("*"), *CurrentName);
		return SuggestedName;
	}
	
	// For patterns without wildcards, just use the pattern as-is
	return Pattern;
}

void FStaticMeshNamingRule::FixAssetNaming(UStaticMesh* StaticMesh, const FString& NewName)
{
	if (!StaticMesh)
	{
		UE_LOG(LogPipelineGuardian, Error, TEXT("FStaticMeshNamingRule::FixAssetNaming: StaticMesh is null"));
		return;
	}

	if (!GEditor)
	{
		UE_LOG(LogPipelineGuardian, Error, TEXT("FStaticMeshNamingRule::FixAssetNaming: GEditor is null"));
		return;
	}

	// Get the editor asset subsystem (UE 5.5 recommended approach)
	UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
	if (!EditorAssetSubsystem)
	{
		UE_LOG(LogPipelineGuardian, Error, TEXT("FStaticMeshNamingRule::FixAssetNaming: Could not get EditorAssetSubsystem"));
		return;
	}

	// Get current asset path
	FString CurrentAssetPath = StaticMesh->GetPackage()->GetName();
	FString CurrentName = StaticMesh->GetName();
	
	// Construct new asset path (same directory, new name)
	FString PackagePath = FPackageName::GetLongPackagePath(CurrentAssetPath);
	FString NewAssetPath = PackagePath + TEXT("/") + NewName;

	// Perform the rename using UE 5.5 API
	bool bRenameSuccess = EditorAssetSubsystem->RenameAsset(CurrentAssetPath, NewAssetPath);
	
	if (bRenameSuccess)
	{
		UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshNamingRule: Successfully renamed '%s' to '%s'"), 
			*CurrentName, *NewName);
	}
	else
	{
		UE_LOG(LogPipelineGuardian, Error, TEXT("FStaticMeshNamingRule: Failed to rename '%s' to '%s'"), 
			*CurrentName, *NewName);
	}
}

#undef LOCTEXT_NAMESPACE 