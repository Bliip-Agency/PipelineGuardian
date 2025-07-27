// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Analysis/IAssetCheckRule.h"

// Forward Declarations
class UStaticMesh;
class UPipelineGuardianProfile;
struct FAssetAnalysisResult;

/**
 * Rule to check if Static Meshes have the minimum required number of LODs.
 * Validates that static mesh assets have sufficient LOD levels for optimization.
 */
class FStaticMeshLODMissingRule : public IAssetCheckRule
{
public:
	FStaticMeshLODMissingRule();
	virtual ~FStaticMeshLODMissingRule() = default;

	// IAssetCheckRule interface
	virtual bool Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults) override;
	virtual FName GetRuleID() const override;
	virtual FText GetRuleDescription() const override;

private:
	/** Get the number of LODs for a static mesh */
	int32 GetStaticMeshLODCount(UStaticMesh* StaticMesh) const;
	
	/** Generate LODs for the static mesh */
	static void GenerateLODs(UStaticMesh* StaticMesh, int32 TargetLODCount);
	
	/** Check if LOD generation is possible for this mesh */
	bool CanGenerateLODs(UStaticMesh* StaticMesh) const;
}; 