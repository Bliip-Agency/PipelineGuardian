#pragma once

#include "CoreMinimal.h"
#include "Analysis/IAssetCheckRule.h"
#include "Engine/StaticMesh.h"

class FStaticMeshMaterialSlotRule : public IAssetCheckRule
{
public:
	FStaticMeshMaterialSlotRule();
	virtual ~FStaticMeshMaterialSlotRule() = default;
	
	virtual bool Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults) override;
	virtual FName GetRuleID() const override;
	virtual FText GetRuleDescription() const override;

private:
	bool HasTooManyMaterialSlots(const UStaticMesh* StaticMesh, int32 WarningThreshold, int32 ErrorThreshold, int32& OutSlotCount) const;
	bool HasEmptyMaterialSlots(const UStaticMesh* StaticMesh, TArray<int32>& OutEmptySlotIndices) const;
	FString GenerateMaterialSlotDescription(const UStaticMesh* StaticMesh, int32 SlotCount, int32 WarningThreshold, int32 ErrorThreshold, const TArray<int32>& EmptySlotIndices) const;
	bool OptimizeMaterialSlots(UStaticMesh* StaticMesh, const TArray<int32>& EmptySlotIndices) const;
	bool CanSafelyOptimizeMaterialSlots(const UStaticMesh* StaticMesh) const;
}; 