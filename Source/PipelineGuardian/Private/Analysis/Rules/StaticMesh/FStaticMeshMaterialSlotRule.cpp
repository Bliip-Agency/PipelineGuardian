#include "FStaticMeshMaterialSlotRule.h"
#include "Analysis/FAssetAnalysisResult.h"
#include "FPipelineGuardianSettings.h"
#include "PipelineGuardian.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSourceData.h"
#include "StaticMeshResources.h"
#include "Misc/MessageDialog.h"

FStaticMeshMaterialSlotRule::FStaticMeshMaterialSlotRule()
{
}

bool FStaticMeshMaterialSlotRule::Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults)
{
	UStaticMesh* StaticMesh = Cast<UStaticMesh>(Asset);
	if (!StaticMesh)
	{
		return false;
	}

	// Get settings
	const UPipelineGuardianSettings* Settings = GetDefault<UPipelineGuardianSettings>();
	if (!Settings || !Settings->bEnableStaticMeshMaterialSlotRule)
	{
		return false;
	}

	bool HasIssue = false;
	EAssetIssueSeverity Severity = Settings->MaterialSlotIssueSeverity;
	FString Description;

	// Check for too many material slots
	int32 SlotCount = 0;
	bool TooManySlots = HasTooManyMaterialSlots(StaticMesh, Settings->MaterialSlotWarningThreshold, Settings->MaterialSlotErrorThreshold, SlotCount);
	
	// Check for empty material slots
	TArray<int32> EmptySlotIndices;
	bool HasEmptySlots = HasEmptyMaterialSlots(StaticMesh, EmptySlotIndices);

	// Determine severity based on issues
	if (TooManySlots)
	{
		HasIssue = true;
		if (SlotCount >= Settings->MaterialSlotErrorThreshold)
		{
			Severity = EAssetIssueSeverity::Error;
		}
		else if (SlotCount >= Settings->MaterialSlotWarningThreshold)
		{
			Severity = EAssetIssueSeverity::Warning;
		}
	}

	if (HasEmptySlots)
	{
		HasIssue = true;
		// Empty slots are always warnings (not critical errors)
		if (Severity == EAssetIssueSeverity::Info)
		{
			Severity = EAssetIssueSeverity::Warning;
		}
	}

	if (HasIssue)
	{
		FAssetAnalysisResult Result;
		Result.Asset = FAssetData(StaticMesh);
		Result.RuleID = GetRuleID();
		Result.Severity = Severity;
		Result.Description = FText::FromString(GenerateMaterialSlotDescription(StaticMesh, SlotCount, Settings->MaterialSlotWarningThreshold, Settings->MaterialSlotErrorThreshold, EmptySlotIndices));

			// Add fix action only for empty slots (not for too many slots)
	// We don't want to auto-remove material slots as they might be important
	if (Settings->bAllowMaterialSlotAutoFix && CanSafelyOptimizeMaterialSlots(StaticMesh) && EmptySlotIndices.Num() > 0)
	{
		Result.FixAction = FSimpleDelegate::CreateLambda([this, StaticMesh, EmptySlotIndices]()
		{
			if (OptimizeMaterialSlots(StaticMesh, EmptySlotIndices))
			{
				UE_LOG(LogPipelineGuardian, Log, TEXT("Successfully removed empty material slots for %s"), *StaticMesh->GetName());
			}
			else
			{
				UE_LOG(LogPipelineGuardian, Warning, TEXT("Failed to remove empty material slots for %s"), *StaticMesh->GetName());
			}
		});
	}

		OutResults.Add(Result);
		return true;
	}

	return false;
}

FName FStaticMeshMaterialSlotRule::GetRuleID() const
{
	return TEXT("SM_MaterialSlot");
}

FText FStaticMeshMaterialSlotRule::GetRuleDescription() const
{
	return FText::FromString(TEXT("Checks for material slot issues including excessive slot count and empty material slots."));
}

bool FStaticMeshMaterialSlotRule::HasTooManyMaterialSlots(const UStaticMesh* StaticMesh, int32 WarningThreshold, int32 ErrorThreshold, int32& OutSlotCount) const
{
	if (!StaticMesh)
	{
		return false;
	}

	OutSlotCount = StaticMesh->GetStaticMaterials().Num();
	return OutSlotCount >= WarningThreshold;
}

bool FStaticMeshMaterialSlotRule::HasEmptyMaterialSlots(const UStaticMesh* StaticMesh, TArray<int32>& OutEmptySlotIndices) const
{
	if (!StaticMesh)
	{
		return false;
	}

	OutEmptySlotIndices.Empty();
	const TArray<FStaticMaterial>& Materials = StaticMesh->GetStaticMaterials();

	for (int32 i = 0; i < Materials.Num(); ++i)
	{
		if (!Materials[i].MaterialInterface)
		{
			OutEmptySlotIndices.Add(i);
		}
	}

	return OutEmptySlotIndices.Num() > 0;
}

FString FStaticMeshMaterialSlotRule::GenerateMaterialSlotDescription(const UStaticMesh* StaticMesh, int32 SlotCount, int32 WarningThreshold, int32 ErrorThreshold, const TArray<int32>& EmptySlotIndices) const
{
	FString Description = FString::Printf(TEXT("Material slot issues detected for %s (%d slots): "), *StaticMesh->GetName(), SlotCount);

	bool HasIssues = false;

	// Check slot count
	if (SlotCount >= ErrorThreshold)
	{
		Description += FString::Printf(TEXT("Too many material slots (%d >= %d error threshold). "), SlotCount, ErrorThreshold);
		HasIssues = true;
	}
	else if (SlotCount >= WarningThreshold)
	{
		Description += FString::Printf(TEXT("High material slot count (%d >= %d warning threshold). "), SlotCount, WarningThreshold);
		HasIssues = true;
	}

	// Check empty slots
	if (EmptySlotIndices.Num() > 0)
	{
		Description += FString::Printf(TEXT("Found %d empty material slot(s) at indices: "), EmptySlotIndices.Num());
		for (int32 i = 0; i < EmptySlotIndices.Num(); ++i)
		{
			Description += FString::FromInt(EmptySlotIndices[i]);
			if (i < EmptySlotIndices.Num() - 1)
			{
				Description += TEXT(", ");
			}
		}
		Description += TEXT(". ");
		
		// Add note about auto-fix availability
		Description += TEXT("Empty slots can be automatically removed with 'Fix Now'.");
		HasIssues = true;
	}

	if (!HasIssues)
	{
		Description = FString::Printf(TEXT("Material slot check failed for %s"), *StaticMesh->GetName());
	}

	return Description;
}

bool FStaticMeshMaterialSlotRule::OptimizeMaterialSlots(UStaticMesh* StaticMesh, const TArray<int32>& EmptySlotIndices) const
{
	if (!StaticMesh || EmptySlotIndices.Num() == 0)
	{
		return false;
	}

	UE_LOG(LogPipelineGuardian, Log, TEXT("Optimizing material slots for %s: removing %d empty slots"), *StaticMesh->GetName(), EmptySlotIndices.Num());

	// Get current materials
	TArray<FStaticMaterial> Materials = StaticMesh->GetStaticMaterials();

	// Remove empty slots (in reverse order to maintain indices)
	for (int32 i = EmptySlotIndices.Num() - 1; i >= 0; --i)
	{
		int32 SlotIndex = EmptySlotIndices[i];
		if (SlotIndex >= 0 && SlotIndex < Materials.Num())
		{
			Materials.RemoveAt(SlotIndex);
			UE_LOG(LogPipelineGuardian, Log, TEXT("Removed empty material slot at index %d"), SlotIndex);
		}
	}

	// Update the static mesh
	StaticMesh->SetStaticMaterials(Materials);

	// Mark for rebuild
	StaticMesh->Build(false);
	StaticMesh->MarkPackageDirty();
	StaticMesh->PostEditChange();

	UE_LOG(LogPipelineGuardian, Log, TEXT("Successfully optimized material slots for %s: %d slots remaining"), *StaticMesh->GetName(), Materials.Num());
	return true;
}

bool FStaticMeshMaterialSlotRule::CanSafelyOptimizeMaterialSlots(const UStaticMesh* StaticMesh) const
{
	if (!StaticMesh)
	{
		return false;
	}

	// Check if mesh has valid geometry
	if (!StaticMesh->GetRenderData() || StaticMesh->GetRenderData()->LODResources.Num() == 0)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("Cannot optimize material slots for %s: No valid geometry"), *StaticMesh->GetName());
		return false;
	}

	// Check if mesh has materials to work with
	const TArray<FStaticMaterial>& Materials = StaticMesh->GetStaticMaterials();
	if (Materials.Num() == 0)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("Cannot optimize material slots for %s: No materials to optimize"), *StaticMesh->GetName());
		return false;
	}

	return true;
} 