// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/SPipelineGuardianWindow.h"
#include "Core/FAssetScanner.h" 
#include "Core/FAssetScanTask.h"
#include "UI/SPipelineGuardianReportView.h" 
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SThrobber.h"
#include "Misc/MessageDialog.h"
#include "PipelineGuardian.h"
#include "FPipelineGuardianSettings.h"
#include "AssetRegistry/AssetData.h"
#include "Analysis/FAssetAnalysisResult.h"
#include "SlateOptMacros.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h" // For specific component casts
#include "Components/SkeletalMeshComponent.h" // For specific component casts
#include "AssetRegistry/AssetRegistryModule.h"
#include "Async/AsyncWork.h"
#include "Async/TaskGraphInterfaces.h" // For AsyncTask
#include "Misc/ScopedSlowTask.h" // For progress dialog
#include "Framework/Application/SlateApplication.h" // For PumpMessages
#include "Analysis/AssetTypeAnalyzers/FStaticMeshAnalyzer.h" // For static mesh analysis
#include "Engine/StaticMesh.h" // For UStaticMesh::StaticClass()

#define LOCTEXT_NAMESPACE "SPipelineGuardianWindow"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SPipelineGuardianWindow::Construct(const FArguments& InArgs)
{
	bIsAnalysisInProgress = false;
	AssetScanner = MakeShareable(new FAssetScanner());

	// Register asset analyzers
	RegisterAssetAnalyzers();

	ChildSlot
	[
		SNew(SVerticalBox)
		// Control Panel Area
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.f)
		[
			SNew(SHorizontalBox)
			// Analyze Project Button
			+ SHorizontalBox::Slot()
			.Padding(2.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("AnalyzeProjectButton", "Analyze Project"))
				.ToolTipText(LOCTEXT("AnalyzeProjectButton_Tooltip", "Analyzes all assets in the current project based on configured rules."))
				.OnClicked(this, &SPipelineGuardianWindow::OnAnalyzeProjectClicked)
				.IsEnabled(this, &SPipelineGuardianWindow::IsAnalysisNotRunning)
			]
			// Analyze Selected Folder Button
			+ SHorizontalBox::Slot()
			.Padding(2.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("AnalyzeFolderButton", "Analyze Selected Folder"))
				.ToolTipText(LOCTEXT("AnalyzeFolderButton_Tooltip", "Analyzes assets within the folder currently selected in the Content Browser."))
				.OnClicked(this, &SPipelineGuardianWindow::OnAnalyzeSelectedFolderClicked)
				.IsEnabled(this, &SPipelineGuardianWindow::IsAnalysisNotRunning)
			]
            // Analyze Open Level Assets Button
            + SHorizontalBox::Slot()
            .Padding(2.f)
            [
                SNew(SButton)
                .Text(LOCTEXT("AnalyzeOpenLevelButton", "Analyze Open Level"))
                .ToolTipText(LOCTEXT("AnalyzeOpenLevelButton_Tooltip", "Analyzes assets referenced in the currently open level."))
                .OnClicked(this, &SPipelineGuardianWindow::OnAnalyzeOpenLevelAssetsClicked)
                .IsEnabled(this, &SPipelineGuardianWindow::IsAnalysisNotRunning)
            ]
			// Analyze Selected Assets Button
			+ SHorizontalBox::Slot()
			.Padding(2.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("AnalyzeSelectedButton", "Analyze Selected Assets"))
				.ToolTipText(LOCTEXT("AnalyzeSelectedButton_Tooltip", "Analyzes assets currently selected in the Content Browser."))
				.OnClicked(this, &SPipelineGuardianWindow::OnAnalyzeSelectedAssetsClicked)
				.IsEnabled(this, &SPipelineGuardianWindow::IsAnalysisNotRunning)
			]
		]
		// Status Bar Area
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.f, 2.f)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.f, 0.f, 5.f, 0.f)
			[
				SAssignNew(AnalysisThrobber, SThrobber)
				.Visibility(EVisibility::Collapsed)
			]
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SAssignNew(StatusTextBlock, STextBlock)
				.Text(LOCTEXT("ReadyStatus", "Ready."))
			]
		]
		// Report View Area
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(5.f)
		[
			SAssignNew(ReportView, SPipelineGuardianReportView)
		]
	];

	// Bind the refresh delegate to re-run the last analysis
	if (ReportView.IsValid())
	{
		ReportView->OnRefreshRequested.BindSP(this, &SPipelineGuardianWindow::OnRefreshRequested);
	}

	SetAnalysisInProgress(false, LOCTEXT("ReadyStatusInitial", "Ready."));
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SPipelineGuardianWindow::RegisterAssetAnalyzers()
{
	if (!AssetScanner.IsValid())
	{
		UE_LOG(LogPipelineGuardian, Error, TEXT("SPipelineGuardianWindow::RegisterAssetAnalyzers: AssetScanner is not valid"));
		return;
	}

	// Register Static Mesh Analyzer
	TSharedPtr<FStaticMeshAnalyzer> StaticMeshAnalyzer = MakeShared<FStaticMeshAnalyzer>();
	AssetScanner->RegisterAssetAnalyzer(UStaticMesh::StaticClass(), StaticMeshAnalyzer);

	UE_LOG(LogPipelineGuardian, Log, TEXT("SPipelineGuardianWindow: Registered asset analyzers"));
}

SPipelineGuardianWindow::~SPipelineGuardianWindow()
{
}

void SPipelineGuardianWindow::SetAnalysisInProgress(bool bInProgress, const FText& StatusMessage)
{
	bIsAnalysisInProgress = bInProgress;
	if (AnalysisThrobber.IsValid())
	{
		AnalysisThrobber->SetVisibility(bInProgress ? EVisibility::Visible : EVisibility::Collapsed);
	}
	if (StatusTextBlock.IsValid())
	{
		StatusTextBlock->SetText(StatusMessage);
	}
}

bool SPipelineGuardianWindow::IsAnalysisNotRunning() const
{
	return !bIsAnalysisInProgress;
}

void SPipelineGuardianWindow::OnRefreshRequested()
{
	UE_LOG(LogPipelineGuardian, Log, TEXT("SPipelineGuardianWindow::OnRefreshRequested: Re-running last analysis"));
	
	// Re-run the last analysis based on stored parameters
	switch (LastAnalysisMode)
	{
		case EAssetScanMode::Project:
			OnAnalyzeProjectClicked();
			break;
		case EAssetScanMode::SelectedFolders:
			OnAnalyzeSelectedFolderClicked();
			break;
		case EAssetScanMode::SelectedAssets:
			OnAnalyzeSelectedAssetsClicked();
			break;
		case EAssetScanMode::OpenLevel:
			OnAnalyzeOpenLevelAssetsClicked();
			break;
		default:
			UE_LOG(LogPipelineGuardian, Warning, TEXT("SPipelineGuardianWindow::OnRefreshRequested: Unknown last analysis mode, defaulting to project analysis"));
			OnAnalyzeProjectClicked();
			break;
	}
}

TArray<TSharedPtr<FAssetAnalysisResult>> ConvertResultsToSharedPointers(const TArray<FAssetAnalysisResult>& Results)
{
	TArray<TSharedPtr<FAssetAnalysisResult>> SharedPtrResults;
	for (const FAssetAnalysisResult& Result : Results)
	{
		SharedPtrResults.Add(MakeShared<FAssetAnalysisResult>(Result));
	}
	return SharedPtrResults;
}

void SPipelineGuardianWindow::OnAssetScanPhaseComplete(
	EAssetScanMode CompletedScanMode,
	const TArray<FString>& CompletedScanParameters,
	const TArray<FAssetData>& DiscoveredAssetsFromTask,
	const FText& TaskCompletionMessage)
{
	// This function is called from a background thread, so we need to dispatch UI updates to the game thread
	AsyncTask(ENamedThreads::GameThread, [this, CompletedScanMode, CompletedScanParameters, DiscoveredAssetsFromTask, TaskCompletionMessage]()
	{
		UE_LOG(LogPipelineGuardian, Log, TEXT("OnAssetScanPhaseComplete: Mode: %d. Task Message: %s"), static_cast<int32>(CompletedScanMode), *TaskCompletionMessage.ToString());

		const UPipelineGuardianSettings* Settings = GetDefault<UPipelineGuardianSettings>();
		if (!AssetScanner.IsValid() || !Settings || !ReportView.IsValid())
		{
			SetAnalysisInProgress(false, LOCTEXT("AnalysisPhaseCompleteErrorInternal", "Error: Could not complete analysis (GT phase) due to internal setup."));
			ReportView->SetResults({});
			return;
		}

		TArray<FAssetData> AssetsToActuallyAnalyze = DiscoveredAssetsFromTask; // Start with what task provided (for Selected/OpenLevel)
		FText FinalOperationSummaryMessage = TaskCompletionMessage;

		// If Project or Folder scan, perform the actual AssetRegistry discovery now on Game Thread
		if (CompletedScanMode == EAssetScanMode::Project)
		{
			SetAnalysisInProgress(true, LOCTEXT("GTPhase_ProjectScan", "Discovering project assets..."));
			AssetScanner->ScanAssetsInPath(TEXT("/Game/"), true, AssetsToActuallyAnalyze);
			FinalOperationSummaryMessage = FText::Format(LOCTEXT("ProjectScanGTDiscoveryCompleteFmt", "Found {0} assets to analyze. Starting detailed analysis..."), AssetsToActuallyAnalyze.Num());
			UE_LOG(LogPipelineGuardian, Log, TEXT("GT Discovery for Project: Found %d assets."), AssetsToActuallyAnalyze.Num());
		}
		else if (CompletedScanMode == EAssetScanMode::SelectedFolders)
		{
			SetAnalysisInProgress(true, FText::Format(LOCTEXT("GTPhase_FolderScan", "Discovering assets in {0} selected folder(s)..."), CompletedScanParameters.Num()));
			AssetsToActuallyAnalyze.Empty(); // Clear to ensure it only contains assets from these paths
			int32 TotalAssetsInFolders = 0;
			for (const FString& Path : CompletedScanParameters)
			{
				TArray<FAssetData> AssetsInPath;
				AssetScanner->ScanAssetsInPath(Path, true, AssetsInPath);
				AssetsToActuallyAnalyze.Append(AssetsInPath);
				TotalAssetsInFolders += AssetsInPath.Num();
			}
			FinalOperationSummaryMessage = FText::Format(LOCTEXT("FolderScanGTDiscoveryCompleteFmt", "Found {0} assets in {1} folder(s). Starting detailed analysis..."), TotalAssetsInFolders, CompletedScanParameters.Num());
			UE_LOG(LogPipelineGuardian, Log, TEXT("GT Discovery for Folders: Found %d assets in %d paths."), TotalAssetsInFolders, CompletedScanParameters.Num());
		}
		else
		{
			// For SelectedAssets or OpenLevel, AssetsToActuallyAnalyze is already set from DiscoveredAssetsFromTask
			// and TaskCompletionMessage is already appropriate for this phase.
			// We might just update the throbber message to reflect UObject loading.
			SetAnalysisInProgress(true, FText::Format(LOCTEXT("GTPhase_PreDiscoveredLoad", "Starting detailed analysis of {0} assets..."), AssetsToActuallyAnalyze.Num()));
		}
		
		TArray<FAssetAnalysisResult> FinalResults;
		if (AssetsToActuallyAnalyze.Num() > 0)
		{
			// Show a progress dialog to inform user about the analysis process
			FText ProgressTitle = LOCTEXT("AnalysisProgressTitle", "Analyzing Assets");
			FText ProgressMessage = FText::Format(LOCTEXT("AnalysisProgressMessage", 
				"Analyzing {0} assets...\n\nThis process may take some time as each asset needs to be loaded and checked.\nThe editor may appear unresponsive during this process, but it is working normally."), 
				AssetsToActuallyAnalyze.Num());
			
			// Create a slow task scope to show progress and allow cancellation
			FScopedSlowTask SlowTask(AssetsToActuallyAnalyze.Num(), ProgressMessage);
			SlowTask.MakeDialog(true); // true = allow cancellation
			
			int32 ProcessedCount = 0;
			for (const FAssetData& AssetData : AssetsToActuallyAnalyze)
			{
				// Check if user cancelled
				if (SlowTask.ShouldCancel())
				{
					FinalOperationSummaryMessage = FText::Format(LOCTEXT("AnalysisCancelledByUser", "Analysis cancelled by user. Processed {0} of {1} assets."), 
						ProcessedCount, AssetsToActuallyAnalyze.Num());
					break;
				}
				
				// Update progress
				SlowTask.EnterProgressFrame(1.0f, FText::Format(LOCTEXT("AnalyzingAssetProgress", "Analyzing: {0}"), 
					FText::FromName(AssetData.AssetName)));
				
				AssetScanner->AnalyzeSingleAsset(AssetData, Settings, FinalResults);
				ProcessedCount++;
				
				// Periodically allow UI updates (every 10 assets)
				if (ProcessedCount % 10 == 0)
				{
					FSlateApplication::Get().PumpMessages();
				}
			}
		}
		else if (CompletedScanMode == EAssetScanMode::Project || CompletedScanMode == EAssetScanMode::SelectedFolders)
		{ 
			// If project/folder scan yielded no assets after GT discovery
			FinalOperationSummaryMessage = FText::Format(LOCTEXT("NoAssetsFoundAfterGTDiscovery", "{0} No assets found to analyze after detailed scan."), FinalOperationSummaryMessage);
		}

		ReportView->SetResults(ConvertResultsToSharedPointers(FinalResults));
		FText OverallCompletionStatus = FText::Format(LOCTEXT("AnalysisFullyCompleteWithDetailsFmt", "{0} Analysis complete. Analyzed {1} assets. {2} issues found."), 
			TaskCompletionMessage, // Original high-level message from task
			AssetsToActuallyAnalyze.Num(), 
			FinalResults.Num()
		);
		SetAnalysisInProgress(false, OverallCompletionStatus);
		UE_LOG(LogPipelineGuardian, Log, TEXT("Analysis fully complete. Final issues: %d"), FinalResults.Num());
	});
}

FReply SPipelineGuardianWindow::OnAnalyzeProjectClicked()
{
	if (bIsAnalysisInProgress) return FReply::Handled();
	
	// Store analysis parameters for refresh functionality
	LastAnalysisMode = EAssetScanMode::Project;
	LastAnalysisParameters.Empty();
	
	SetAnalysisInProgress(true, LOCTEXT("StartingProjectAnalysisAsyncPhase", "Starting project analysis: Initializing async task..."));

	const UPipelineGuardianSettings* Settings = GetDefault<UPipelineGuardianSettings>();
	if (!AssetScanner.IsValid() || !Settings || !ReportView.IsValid())
	{
		SetAnalysisInProgress(false, LOCTEXT("ProjectAnalysisErrorInternalPreAsync", "Error: Could not start project analysis (pre-async).."));
		return FReply::Handled();
	}
	if (!Settings->bMasterSwitch_EnableAnalysis)
	{
		ReportView->SetResults({}); 
		SetAnalysisInProgress(false, LOCTEXT("AnalysisDisabledMasterProjectPreAsync", "Analysis is globally disabled. Report cleared."));
		return FReply::Handled();
	}

	TArray<FString> ScanParameters; 
	TArray<FAssetData> PreDiscoveredAssets;

	(new FAutoDeleteAsyncTask<FAssetScanTask>(
		EAssetScanMode::Project, 
		ScanParameters, 
		PreDiscoveredAssets,
		AssetScanner, 
		Settings, 
		FAssetScanCompletionDelegate::CreateSP(this, &SPipelineGuardianWindow::OnAssetScanPhaseComplete)
	))->StartBackgroundTask();
	
	return FReply::Handled();
}

FReply SPipelineGuardianWindow::OnAnalyzeSelectedFolderClicked()
{
	if (bIsAnalysisInProgress) return FReply::Handled();
	
	SetAnalysisInProgress(true, LOCTEXT("StartingFolderAnalysisAsyncPhase", "Starting selected folder analysis: Initializing async task..."));

	const UPipelineGuardianSettings* Settings = GetDefault<UPipelineGuardianSettings>();
	if (!AssetScanner.IsValid() || !Settings || !ReportView.IsValid())
	{
		SetAnalysisInProgress(false, LOCTEXT("FolderAnalysisErrorInternalPreAsync", "Error: Could not start folder analysis (pre-async)."));
		return FReply::Handled();
	}
	if (!Settings->bMasterSwitch_EnableAnalysis)
	{
		ReportView->SetResults({});
		SetAnalysisInProgress(false, LOCTEXT("AnalysisDisabledMasterFolderPreAsync", "Analysis is globally disabled. Report cleared."));
		return FReply::Handled();
	}

	IContentBrowserSingleton& ContentBrowserSingleton = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser")).Get();
	TArray<FString> SelectedPaths;
	ContentBrowserSingleton.GetSelectedPathViewFolders(SelectedPaths);

	if (SelectedPaths.Num() == 0)
	{
		ReportView->SetResults({}); 
		SetAnalysisInProgress(false, LOCTEXT("NoFolderSelectedForAnalysisPreAsync", "No folder selected. Report cleared."));
		return FReply::Handled();
	}

	// Store analysis parameters for refresh functionality
	LastAnalysisMode = EAssetScanMode::SelectedFolders;
	LastAnalysisParameters = SelectedPaths;

	TArray<FAssetData> PreDiscoveredAssets; 

	(new FAutoDeleteAsyncTask<FAssetScanTask>(
		EAssetScanMode::SelectedFolders, 
		SelectedPaths, 
		PreDiscoveredAssets,
		AssetScanner, 
		Settings, 
		FAssetScanCompletionDelegate::CreateSP(this, &SPipelineGuardianWindow::OnAssetScanPhaseComplete)
	))->StartBackgroundTask();

	return FReply::Handled();
}

FReply SPipelineGuardianWindow::OnAnalyzeOpenLevelAssetsClicked()
{
	if (bIsAnalysisInProgress) return FReply::Handled();
	
	// Store analysis parameters for refresh functionality
	LastAnalysisMode = EAssetScanMode::OpenLevel;
	LastAnalysisParameters.Empty();
	
	SetAnalysisInProgress(true, LOCTEXT("StartingOpenLevelAnalysisAsyncPhase", "Starting open level analysis: Discovering assets (GT) & initializing task..."));

	const UPipelineGuardianSettings* Settings = GetDefault<UPipelineGuardianSettings>();
	if (!AssetScanner.IsValid() || !Settings || !ReportView.IsValid())
	{
		SetAnalysisInProgress(false, LOCTEXT("OpenLevelAnalysisErrorInternalPreAsync", "Error: Could not start open level analysis (pre-async)."));
		return FReply::Handled();
	}
	if (!Settings->bMasterSwitch_EnableAnalysis)
	{
		ReportView->SetResults({});
		SetAnalysisInProgress(false, LOCTEXT("AnalysisDisabledMasterOpenLevelPreAsync", "Analysis is globally disabled. Report cleared."));
		return FReply::Handled();
	}

	UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!EditorWorld)
	{
		ReportView->SetResults({});
		SetAnalysisInProgress(false, LOCTEXT("NoEditorWorldOpenLevelPreAsync", "No active editor world. Report cleared."));
		return FReply::Handled();
	}

	TArray<FAssetData> AssetsToAnalyze; // Pre-discovered on Game Thread
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get(); // Get the interface

	if (EditorWorld->GetCurrentLevel())
	{
		for (AActor* Actor : EditorWorld->GetCurrentLevel()->Actors)
		{
			if (!Actor) continue;
			FAssetData ActorAssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(Actor->GetClass())); // Use AssetRegistry interface
			if (ActorAssetData.IsValid() && ActorAssetData.IsUAsset()) AssetsToAnalyze.AddUnique(ActorAssetData);
			TArray<UActorComponent*> Components;
			Actor->GetComponents(Components);
			for (UActorComponent* Component : Components)
			{
				if (!Component) continue;
				if (UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(Component))
				{
					if (StaticMeshComp->GetStaticMesh())
					{
						FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(StaticMeshComp->GetStaticMesh())); // Use AssetRegistry interface
						if (AssetData.IsValid() && AssetData.IsUAsset()) AssetsToAnalyze.AddUnique(AssetData);
						for (int32 MatIdx = 0; MatIdx < StaticMeshComp->GetNumMaterials(); ++MatIdx)
						{
							UMaterialInterface* Material = StaticMeshComp->GetMaterial(MatIdx);
							if (Material) { FAssetData MatAssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(Material)); if (MatAssetData.IsValid() && MatAssetData.IsUAsset()) AssetsToAnalyze.AddUnique(MatAssetData); } // Use AssetRegistry interface
						}
					}
				}
				else if (USkeletalMeshComponent* SkelMeshComp = Cast<USkeletalMeshComponent>(Component))
				{ 
					if (SkelMeshComp->GetSkeletalMeshAsset())
					{
						FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(SkelMeshComp->GetSkeletalMeshAsset())); // Use AssetRegistry interface
						if (AssetData.IsValid() && AssetData.IsUAsset()) AssetsToAnalyze.AddUnique(AssetData);
					}
					for (int32 MatIdx = 0; MatIdx < SkelMeshComp->GetNumMaterials(); ++MatIdx)
					{
						UMaterialInterface* Material = SkelMeshComp->GetMaterial(MatIdx);
						if (Material) { FAssetData MatAssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(Material)); if (MatAssetData.IsValid() && MatAssetData.IsUAsset()) AssetsToAnalyze.AddUnique(MatAssetData); } // Use AssetRegistry interface
					}
				}
			}
		}
	}
	else
	{
		ReportView->SetResults({});
		SetAnalysisInProgress(false, LOCTEXT("NoCurrentLevelOpenLevelPreAsync", "No current level in world. Report cleared."));
		return FReply::Handled();
	}

	UE_LOG(LogPipelineGuardian, Log, TEXT("GT Pre-discovered %d assets for Open Level scan."), AssetsToAnalyze.Num());
	if (AssetsToAnalyze.Num() == 0)
	{
		ReportView->SetResults({});
		SetAnalysisInProgress(false, LOCTEXT("NoAssetsInOpenLevelForAsyncPreAsync", "No scannable assets in open level. Report cleared."));
		return FReply::Handled();
	}

	TArray<FString> ScanParameters; 

	(new FAutoDeleteAsyncTask<FAssetScanTask>(
		EAssetScanMode::OpenLevel, 
		ScanParameters, 
		AssetsToAnalyze, 
		AssetScanner, 
		Settings, 
		FAssetScanCompletionDelegate::CreateSP(this, &SPipelineGuardianWindow::OnAssetScanPhaseComplete)
	))->StartBackgroundTask();

	return FReply::Handled();
}

FReply SPipelineGuardianWindow::OnAnalyzeSelectedAssetsClicked()
{
	if (bIsAnalysisInProgress) return FReply::Handled();
	
	// Store analysis parameters for refresh functionality
	LastAnalysisMode = EAssetScanMode::SelectedAssets;
	LastAnalysisParameters.Empty();
	
	SetAnalysisInProgress(true, LOCTEXT("StartingSelectedAssetsAsyncPhase", "Starting selected assets analysis: Discovering assets (GT) & initializing task..."));

	const UPipelineGuardianSettings* Settings = GetDefault<UPipelineGuardianSettings>();
	if (!AssetScanner.IsValid() || !Settings || !ReportView.IsValid())
	{
		SetAnalysisInProgress(false, LOCTEXT("SelectedAssetsAnalysisErrorInternalPreAsync", "Error: Could not start selected assets analysis (pre-async)."));
		return FReply::Handled();
	}
	if (!Settings->bMasterSwitch_EnableAnalysis)
	{
		ReportView->SetResults({});
		SetAnalysisInProgress(false, LOCTEXT("AnalysisDisabledMasterSelectedPreAsync", "Analysis is globally disabled. Report cleared."));
		return FReply::Handled();
	}

	TArray<FAssetData> SelectedAssetsToAnalyze; // Pre-discovered on Game Thread
	AssetScanner->ScanSelectedAssets(SelectedAssetsToAnalyze);
	UE_LOG(LogPipelineGuardian, Log, TEXT("GT Pre-discovered %d selected assets for Analysis."), SelectedAssetsToAnalyze.Num());

	if (SelectedAssetsToAnalyze.Num() == 0)
	{
		ReportView->SetResults({});
		SetAnalysisInProgress(false, LOCTEXT("NoAssetsSelectedForAnalysisAsyncPreAsync", "No assets selected. Report cleared."));
		return FReply::Handled();
	}

	TArray<FString> ScanParameters; 

	(new FAutoDeleteAsyncTask<FAssetScanTask>(
		EAssetScanMode::SelectedAssets, 
		ScanParameters, 
		SelectedAssetsToAnalyze, 
		AssetScanner, 
		Settings, 
		FAssetScanCompletionDelegate::CreateSP(this, &SPipelineGuardianWindow::OnAssetScanPhaseComplete)
	))->StartBackgroundTask();

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE 