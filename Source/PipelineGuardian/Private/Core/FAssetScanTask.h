// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Async/AsyncWork.h"
#include "AssetRegistry/AssetData.h"
#include "HAL/PlatformTime.h" // Required for FPlatformTime::Seconds()

// Forward declarations
class FAssetScanner;
class UPipelineGuardianSettings;
struct FAssetAnalysisResult;

// Enum to define the scope of the asset scan
enum class EAssetScanMode : uint8
{
    Project,
    SelectedFolders,
    SelectedAssets,
    OpenLevel
};

// Delegate to be called on the game thread when the scan task's phase is complete.
// For Project/Folder modes, AssetsFromTask might be empty, and GT handler uses ScanMode + ScanParameters to discover.
// For Selected/OpenLevel modes, AssetsFromTask contains pre-discovered assets.
DECLARE_DELEGATE_FourParams(FAssetScanCompletionDelegate, EAssetScanMode /*ScanMode*/, const TArray<FString>& /*ScanParameters*/, const TArray<FAssetData>& /*AssetsFromTask*/, const FText& /*TaskCompletionMessage*/);

class FAssetScanTask : public FNonAbandonableTask
{
public:
    FAssetScanTask(
        EAssetScanMode InScanMode,
        TArray<FString> InScanParameters, 
        TArray<FAssetData> InPreDiscoveredAssets, 
        TWeakPtr<FAssetScanner> InAssetScannerPtr,
        const UPipelineGuardianSettings* InSettings, 
        FAssetScanCompletionDelegate InOnCompletionDelegate
    );

    /** Performs the async work. For Project/Folder, this task doesn't do AssetRegistry discovery. */
    void DoWork();

    /** Required by TStatId, returns a unique ID for this task type for profiling. */
    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(FAssetScanTask, STATGROUP_ThreadPoolAsyncTasks);
    }

private:
    EAssetScanMode ScanMode;
    TArray<FString> ScanParameters;
    TArray<FAssetData> PreDiscoveredAssets; 
    TWeakPtr<FAssetScanner> AssetScannerPtr;
    const UPipelineGuardianSettings* Settings;
    FAssetScanCompletionDelegate OnCompletionDelegate;
    
    // Data prepared by DoWork() to pass to the delegate
    TArray<FAssetData> AssetsToPassToDelegate; // Will be PreDiscoveredAssets or empty
    FText TaskCompletionMessage;
    double StartTime = 0.0;
}; 