#pragma once

#include "CoreMinimal.h"
#include "Analysis/IAssetCheckRule.h"
#include "Engine/StaticMesh.h"

class FStaticMeshLightmapResolutionRule : public IAssetCheckRule
{
public:
	FStaticMeshLightmapResolutionRule();
	virtual ~FStaticMeshLightmapResolutionRule() = default;
	
	virtual bool Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults) override;
	virtual FName GetRuleID() const override;
	virtual FText GetRuleDescription() const override;

private:
	bool HasInappropriateLightmapResolution(const UStaticMesh* StaticMesh, int32 MinResolution, int32 MaxResolution, int32& OutCurrentResolution) const;
	FString GenerateLightmapResolutionDescription(const UStaticMesh* StaticMesh, int32 CurrentResolution, int32 MinResolution, int32 MaxResolution) const;
	bool SetOptimalLightmapResolution(UStaticMesh* StaticMesh, int32 MinResolution, int32 MaxResolution) const;
	bool CanSafelySetLightmapResolution(const UStaticMesh* StaticMesh) const;
	int32 CalculateOptimalLightmapResolution(const UStaticMesh* StaticMesh, int32 MinResolution, int32 MaxResolution) const;
}; 