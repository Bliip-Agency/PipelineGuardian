// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/FAssetScanTask.h"
#include "Core/FAssetScanner.h" // Only for TWeakPtr validation, not direct scanning
#include "FPipelineGuardianSettings.h"
#include "PipelineGuardian.h" // For LogPipelineGuardian
// Removed AssetRegistry, ContentBrowser, Editor, Engine/World, Engine/Level, GameFramework/Actor includes as discovery is deferred
#include "HAL/PlatformTime.h"

#define LOCTEXT_NAMESPACE "FAssetScanTask"

FAssetScanTask::FAssetScanTask(
    EAssetScanMode InScanMode,
    TArray<FString> InScanParameters,
    TArray<FAssetData> InPreDiscoveredAssets,
    TWeakPtr<FAssetScanner> InAssetScannerPtr,
    const UPipelineGuardianSettings* InSettings,
    FAssetScanCompletionDelegate InOnCompletionDelegate
) :
    ScanMode(InScanMode),
    ScanParameters(MoveTemp(InScanParameters)),
    PreDiscoveredAssets(MoveTemp(InPreDiscoveredAssets)),
    AssetScannerPtr(InAssetScannerPtr),
    Settings(InSettings), // Still needed for context, though direct use in DoWork is minimal
    OnCompletionDelegate(InOnCompletionDelegate),
    StartTime(FPlatformTime::Seconds())
{
    UE_LOG(LogPipelineGuardian, Log, TEXT("FAssetScanTask created. Mode: %d"), static_cast<int32>(ScanMode));
}

void FAssetScanTask::DoWork()
{
    UE_LOG(LogPipelineGuardian, Log, TEXT("FAssetScanTask::DoWork() starting background phase..."));
    AssetsToPassToDelegate.Empty();

    // Sanity check pointers, though direct use of Scanner/Settings is now minimal in DoWork
    TSharedPtr<FAssetScanner> Scanner = AssetScannerPtr.Pin();
    if (!Scanner.IsValid() || !Settings)
    {
        TaskCompletionMessage = LOCTEXT("ScannerOrSettingsInvalidInTask", "Asset scanner or settings became invalid during async task background phase.");
        if (OnCompletionDelegate.IsBound())
        {
            // Pass original ScanMode and Parameters so GT handler knows what failed.
            OnCompletionDelegate.Execute(ScanMode, ScanParameters, AssetsToPassToDelegate, TaskCompletionMessage);
        }
        return;
    }



    switch (ScanMode)
    {
        case EAssetScanMode::Project:
            TaskCompletionMessage = LOCTEXT("ProjectScanPendingGT", "Project asset discovery pending on Game Thread...");

            break;

        case EAssetScanMode::SelectedFolders:
            TaskCompletionMessage = FText::Format(LOCTEXT("FolderScanPendingGTFmt", "Selected folder(s) asset discovery pending on Game Thread for {0} path(s)..."), ScanParameters.Num());

            break;

        case EAssetScanMode::SelectedAssets:
            AssetsToPassToDelegate = PreDiscoveredAssets;
            TaskCompletionMessage = FText::Format(LOCTEXT("SelectedAssetsReadyForAnalysisFmt", "{0} selected asset(s) ready for analysis."), AssetsToPassToDelegate.Num());
            break;

        case EAssetScanMode::OpenLevel:
            AssetsToPassToDelegate = PreDiscoveredAssets;
            TaskCompletionMessage = FText::Format(LOCTEXT("OpenLevelAssetsReadyForAnalysisFmt", "{0} open level asset(s) ready for analysis."), AssetsToPassToDelegate.Num());
            break;

        default:
            TaskCompletionMessage = LOCTEXT("UnknownScanModeInTask", "Unknown scan mode in async task.");
            break;
    }

    double ElapsedTime = FPlatformTime::Seconds() - StartTime;
    UE_LOG(LogPipelineGuardian, Log, TEXT("FAssetScanTask::DoWork() background phase finished in %.2f seconds. Message: %s"), ElapsedTime, *TaskCompletionMessage.ToString());

    if (OnCompletionDelegate.IsBound())
    {
        OnCompletionDelegate.Execute(ScanMode, ScanParameters, AssetsToPassToDelegate, TaskCompletionMessage);
    }
    else
    {
        UE_LOG(LogPipelineGuardian, Warning, TEXT("FAssetScanTask: OnCompletionDelegate was not bound!"));
    }
}

#undef LOCTEXT_NAMESPACE 