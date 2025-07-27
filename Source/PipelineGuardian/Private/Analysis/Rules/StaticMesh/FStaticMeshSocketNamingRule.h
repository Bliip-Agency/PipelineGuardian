#pragma once

#include "CoreMinimal.h"
#include "Analysis/IAssetCheckRule.h"
#include "Engine/StaticMesh.h"

class FStaticMeshSocketNamingRule : public IAssetCheckRule
{
public:
	FStaticMeshSocketNamingRule();
	virtual ~FStaticMeshSocketNamingRule() = default;
	
	virtual bool Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults) override;
	virtual FName GetRuleID() const override;
	virtual FText GetRuleDescription() const override;

private:
	bool HasInvalidSocketNaming(const UStaticMesh* StaticMesh, const FString& RequiredPrefix, TArray<FString>& OutInvalidSocketNames) const;
	bool HasInvalidSocketTransforms(const UStaticMesh* StaticMesh, float WarningDistance, TArray<FString>& OutInvalidSocketNames) const;
	FString GenerateSocketNamingDescription(const UStaticMesh* StaticMesh, const TArray<FString>& InvalidNamingSockets, const TArray<FString>& InvalidTransformSockets, const FString& RequiredPrefix) const;
	bool FixSocketIssues(UStaticMesh* StaticMesh, const FString& RequiredPrefix) const;
	bool CanSafelyFixSocketIssues(const UStaticMesh* StaticMesh) const;
}; 