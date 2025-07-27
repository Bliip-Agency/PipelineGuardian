#pragma once

#include "CoreMinimal.h"
#include "Analysis/IAssetCheckRule.h"
#include "Analysis/FAssetAnalysisResult.h"
#include "FPipelineGuardianSettings.h"

class UStaticMesh;

/**
 * Rule to check for scaling issues in static meshes
 * - Non-uniform scaling detection
 * - Zero scale detection
 * - Extreme scale values
 */
class FStaticMeshScalingRule : public IAssetCheckRule
{
public:
	FStaticMeshScalingRule();
	virtual ~FStaticMeshScalingRule() = default;

	// IAssetCheckRule interface
	virtual bool Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults) override;
	virtual FName GetRuleID() const override;
	virtual FText GetRuleDescription() const override;

private:
	/**
	 * Check if mesh has non-uniform scaling
	 * @param StaticMesh The mesh to check
	 * @param WarningRatio Threshold ratio for warning
	 * @param OutScale Output scale values
	 * @return True if non-uniform scaling detected
	 */
	bool HasNonUniformScale(const UStaticMesh* StaticMesh, float WarningRatio, FVector& OutScale) const;

	/**
	 * Check if mesh has zero or near-zero scale components
	 * @param StaticMesh The mesh to check
	 * @param Threshold Minimum scale threshold
	 * @param OutZeroScaleAxes Output axes with zero scale
	 * @return True if zero scale detected
	 */
	bool HasZeroScale(const UStaticMesh* StaticMesh, float Threshold, TArray<FString>& OutZeroScaleAxes) const;

	/**
	 * Check if mesh has extreme scale values from DCC tools
	 * @param StaticMesh The mesh to check
	 * @param OutIssues Output issues found
	 * @return True if extreme scale detected
	 */
	bool HasExtremeScaleValues(const UStaticMesh* StaticMesh, TArray<FString>& OutIssues) const;

	/**
	 * Generate description for non-uniform scaling issues
	 */
	FString GenerateNonUniformScaleDescription(const UStaticMesh* StaticMesh, const FVector& Scale, float WarningRatio) const;

	/**
	 * Generate description for zero scale issues
	 */
	FString GenerateZeroScaleDescription(const UStaticMesh* StaticMesh, const TArray<FString>& ZeroScaleAxes) const;

	/**
	 * Generate description for extreme scale issues
	 */
	FString GenerateExtremeScaleDescription(const UStaticMesh* StaticMesh, const TArray<FString>& Issues) const;

	/**
	 * Check if scaling fix can be safely applied
	 */
	bool CanSafelyFixScaling(const UStaticMesh* StaticMesh) const;

	/**
	 * Fix zero scale issues
	 */
	bool FixZeroScale(UStaticMesh* StaticMesh, const TArray<FString>& AxesToFix) const;
}; 