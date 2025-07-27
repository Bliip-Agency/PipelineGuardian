#pragma once

#include "CoreMinimal.h"
#include "Analysis/IAssetCheckRule.h"
#include "Engine/StaticMesh.h"

class FStaticMeshNaniteSuitabilityRule : public IAssetCheckRule
{
public:
	FStaticMeshNaniteSuitabilityRule();
	virtual ~FStaticMeshNaniteSuitabilityRule() = default;
	
	virtual bool Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults) override;
	virtual FName GetRuleID() const override;
	virtual FText GetRuleDescription() const override;

private:
	bool ShouldUseNanite(const UStaticMesh* StaticMesh, int32 SuitabilityThreshold, int32 DisableThreshold) const;
	bool ShouldDisableNanite(const UStaticMesh* StaticMesh, int32 DisableThreshold) const;
	FString GenerateNaniteSuitabilityDescription(const UStaticMesh* StaticMesh, bool ShouldUseNanite, bool ShouldDisable, int32 TriangleCount) const;
	bool OptimizeNaniteSettings(UStaticMesh* StaticMesh, bool ShouldUseNanite) const;
	bool CanSafelyOptimizeNanite(const UStaticMesh* StaticMesh) const;
}; 