#pragma once

#include "CoreMinimal.h"
#include "Analysis/IAssetCheckRule.h"
#include "Analysis/FAssetAnalysisResult.h"
#include "FPipelineGuardianSettings.h"

class UStaticMesh;

/**
 * Rule to check for transform and pivot issues in static meshes
 * - Off-origin pivot detection (relative to mesh bounds)
 * - DCC transformation detection
 * - Asset-type specific pivot validation
 */
class FStaticMeshTransformRule : public IAssetCheckRule
{
public:
	FStaticMeshTransformRule();
	virtual ~FStaticMeshTransformRule() = default;

	// IAssetCheckRule interface
	virtual bool Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults) override;
	virtual FName GetRuleID() const override;
	virtual FText GetRuleDescription() const override;

private:
	/**
	 * Check if mesh has problematic pivot offset relative to mesh bounds
	 * @param StaticMesh The mesh to check
	 * @param WarningDistance Distance threshold for warning
	 * @param ErrorDistance Distance threshold for error
	 * @param OutPivotOffset Output pivot offset
	 * @return True if problematic pivot detected
	 */
	bool HasProblematicPivot(const UStaticMesh* StaticMesh, float WarningDistance, float ErrorDistance, FVector& OutPivotOffset) const;

	/**
	 * Check if mesh has unapplied DCC transformations
	 * @param StaticMesh The mesh to check
	 * @param OutIssues Output issues found
	 * @return True if DCC transformations detected
	 */
	bool HasUnappliedDCCTransformations(const UStaticMesh* StaticMesh, TArray<FString>& OutIssues) const;

	/**
	 * Generate description for transform pivot issues
	 */
	FString GenerateTransformPivotDescription(const UStaticMesh* StaticMesh, const FVector& PivotOffset, float WarningDistance, float ErrorDistance) const;

	/**
	 * Generate description for DCC transformation issues
	 */
	FString GenerateDCCTransformDescription(const UStaticMesh* StaticMesh, const TArray<FString>& Issues) const;

	/**
	 * Check if transform fix can be safely applied
	 */
	bool CanSafelyFixTransform(const UStaticMesh* StaticMesh) const;

	/**
	 * Fix transform pivot issues
	 */
	bool FixTransformPivot(UStaticMesh* StaticMesh) const;
}; 