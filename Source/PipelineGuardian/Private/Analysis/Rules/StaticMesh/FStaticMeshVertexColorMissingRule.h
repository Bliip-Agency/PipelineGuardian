#pragma once

#include "CoreMinimal.h"
#include "Analysis/IAssetCheckRule.h"
#include "Engine/StaticMesh.h"

class FStaticMeshVertexColorMissingRule : public IAssetCheckRule
{
public:
	FStaticMeshVertexColorMissingRule();
	virtual ~FStaticMeshVertexColorMissingRule() = default;
	
	virtual bool Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults) override;
	virtual FName GetRuleID() const override;
	virtual FText GetRuleDescription() const override;

private:
	bool HasMissingVertexColors(const UStaticMesh* StaticMesh, int32 RequiredThreshold) const;
	bool HasUnusedVertexColorChannels(const UStaticMesh* StaticMesh, TArray<FString>& OutUnusedChannels) const;
	bool HasMissingRequiredChannels(const UStaticMesh* StaticMesh, const FString& RequiredChannels, TArray<FString>& OutMissingChannels) const;
	FString GenerateVertexColorMissingDescription(const UStaticMesh* StaticMesh, int32 TriangleCount, int32 RequiredThreshold) const;
	FString GenerateUnusedChannelDescription(const UStaticMesh* StaticMesh, const TArray<FString>& UnusedChannels) const;
	FString GenerateMissingChannelDescription(const UStaticMesh* StaticMesh, const TArray<FString>& MissingChannels) const;
	bool GenerateVertexColors(UStaticMesh* StaticMesh) const;
	bool CanSafelyGenerateVertexColors(const UStaticMesh* StaticMesh) const;
	bool OptimizeVertexColors(UStaticMesh* StaticMesh, const TArray<FString>& ChannelsToRemove) const;
}; 