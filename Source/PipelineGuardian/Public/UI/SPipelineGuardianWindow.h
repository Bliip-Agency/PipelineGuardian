// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Templates/SharedPointer.h" // For TSharedPtr
#include "Core/FAssetScanTask.h" // For EAssetScanMode and FAssetScanCompletionDelegate (if not already via CoreMinimal/indirectly)

// Forward Declarations
class FAssetScanner;
class SPipelineGuardianReportView;
class SThrobber;
class STextBlock;
// FAssetScanTask is now included above

// Delegate for FAssetScanTask completion - Ensure this matches FAssetScanTask.h
// DECLARE_DELEGATE_FourParams(FAssetScanCompletionDelegate, EAssetScanMode, const TArray<FString>&, const TArray<FAssetData>&, const FText&);
// ^ This is already declared in FAssetScanTask.h and should be picked up.
// If not, it indicates an include ordering or UHT issue, but we assume it is.

class SPipelineGuardianWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPipelineGuardianWindow)
	{}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	/** Destructor */
	~SPipelineGuardianWindow();

	/** Pointer to the asset scanner utility. */
	TSharedPtr<FAssetScanner> AssetScanner;

	/** Pointer to the report view widget. */
	TSharedPtr<SPipelineGuardianReportView> ReportView;

	/** Throbber widget for indicating progress. */
	TSharedPtr<SThrobber> AnalysisThrobber;

	/** Text block for displaying status messages. */
	TSharedPtr<STextBlock> StatusTextBlock;

	/** Flag to indicate if an analysis is currently in progress */
	bool bIsAnalysisInProgress = false;

	/** Last analysis mode for refresh functionality */
	EAssetScanMode LastAnalysisMode = EAssetScanMode::Project;
	
	/** Last analysis parameters for refresh functionality */
	TArray<FString> LastAnalysisParameters;

	/** Updates the UI to reflect the analysis state (in progress or finished) */
	void SetAnalysisInProgress(bool bInProgress, const FText& StatusMessage = FText::GetEmpty());

	/** Returns true if an analysis is not currently running, used for button enabled state */
	bool IsAnalysisNotRunning() const;

	/** Called when the report view requests a refresh after fixes are applied */
	void OnRefreshRequested();

private:
	/** Registers all asset analyzers with the asset scanner */
	void RegisterAssetAnalyzers();
	/** Handler for when the asset scan async task completes its phase. */
	void OnAssetScanPhaseComplete(
		EAssetScanMode CompletedScanMode,
		const TArray<FString>& CompletedScanParameters,
		const TArray<FAssetData>& DiscoveredAssetsFromTask,
		const FText& TaskCompletionMessage
	);

	//~ Begin Button Click Handlers
	FReply OnAnalyzeProjectClicked();
	FReply OnAnalyzeSelectedFolderClicked();
	FReply OnAnalyzeSelectedAssetsClicked();
	FReply OnAnalyzeOpenLevelAssetsClicked();
	//~ End Button Click Handlers
}; 