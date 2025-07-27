// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Analysis/IAssetCheckRule.h"

// Forward Declarations
class UStaticMesh;
class UPipelineGuardianProfile;
struct FAssetAnalysisResult;
enum class EAssetIssueSeverity : uint8;

/**
 * Rule to check if Static Meshes have sufficient polygon reduction between LOD levels.
 * Validates that each LOD level has appropriate polygon count reduction for performance optimization.
 */
class FStaticMeshLODPolyReductionRule : public IAssetCheckRule
{
public:
	FStaticMeshLODPolyReductionRule();
	virtual ~FStaticMeshLODPolyReductionRule() = default;

	// IAssetCheckRule interface
	virtual bool Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults) override;
	virtual FName GetRuleID() const override;
	virtual FText GetRuleDescription() const override;

private:
	/** Get the triangle count for a specific LOD level */
	int32 GetLODTriangleCount(UStaticMesh* StaticMesh, int32 LODIndex) const;
	
	/** Calculate the reduction percentage between two LOD levels */
	float CalculateReductionPercentage(int32 HigherLODTriangles, int32 LowerLODTriangles) const;
	
	/** Fix LOD reduction by regenerating LODs with proper reduction settings */
	static void FixLODReduction(UStaticMesh* StaticMesh, int32 ProblematicLODIndex, float TargetReductionPercentage);
	
	/** Fix all problematic LODs in one comprehensive operation */
	static void FixAllLODReductions(UStaticMesh* StaticMesh, const TArray<int32>& ProblematicLODs, float TargetReductionPercentage);
	
	/** Check if LOD reduction can be fixed for this mesh */
	bool CanFixLODReduction(UStaticMesh* StaticMesh) const;
	
	/** Get severity level based on reduction percentage deficit */
	EAssetIssueSeverity GetSeverityForReduction(float ActualReduction, float MinReduction, float WarningThreshold, float ErrorThreshold) const;
}; 