// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Analysis/IAssetCheckRule.h"

// Forward Declarations
class UStaticMesh;
class UPipelineGuardianProfile;
struct FAssetAnalysisResult;
enum class ELightmapUVChannelStrategy : uint8;

/**
 * Rule to check if Static Meshes have proper lightmap UV configuration.
 * Validates that static mesh assets either have bGenerateLightmapUVs enabled
 * or have a valid UV channel 1 for lightmapping.
 */
class FStaticMeshLightmapUVMissingRule : public IAssetCheckRule
{
public:
	FStaticMeshLightmapUVMissingRule();
	virtual ~FStaticMeshLightmapUVMissingRule() = default;

	// IAssetCheckRule interface
	virtual bool Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults) override;
	virtual FName GetRuleID() const override;
	virtual FText GetRuleDescription() const override;

private:
	/** Check if the static mesh has valid UV channel 1 for lightmapping */
	bool HasValidLightmapUVChannel(UStaticMesh* StaticMesh) const;
	
	/** Get the number of UV channels for LOD 0 */
	int32 GetUVChannelCount(UStaticMesh* StaticMesh) const;
	
	/** Check if UV channel 1 has valid UVs (not overlapping/degenerate) */
	bool IsUVChannelValid(UStaticMesh* StaticMesh, int32 UVChannelIndex) const;
	
	/** Generate lightmap UVs for the static mesh */
	static void GenerateLightmapUVs(UStaticMesh* StaticMesh);
	
	/** Enable bGenerateLightmapUVs flag for the static mesh */
	static void EnableGenerateLightmapUVs(UStaticMesh* StaticMesh, int32 DestinationUVChannel = 1);
	
	/** Determine the optimal UV channel for lightmap generation based on strategy */
	static int32 DetermineOptimalLightmapUVChannel(UStaticMesh* StaticMesh, ELightmapUVChannelStrategy Strategy, int32 PreferredChannel);
	
	/** Find the next available UV channel that doesn't contain existing UV data */
	static int32 FindNextAvailableUVChannel(UStaticMesh* StaticMesh, int32 StartFromChannel = 1);
	
	/** Check if a UV channel contains valid UV data */
	static bool HasValidUVData(UStaticMesh* StaticMesh, int32 UVChannel);
	
	/** Check if lightmap UV generation is possible for this mesh */
	bool CanGenerateLightmapUVs(UStaticMesh* StaticMesh) const;
	
	/** Get the lightmap coordinate index for the mesh */
	int32 GetLightmapCoordinateIndex(UStaticMesh* StaticMesh) const;
}; 