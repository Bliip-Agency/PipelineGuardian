#pragma once

#include "CoreMinimal.h"
#include "Analysis/IAssetCheckRule.h"

// Forward declarations
class UStaticMesh;
class UPipelineGuardianProfile;
struct FAssetAnalysisResult;
enum class EAssetIssueSeverity : uint8;

/**
 * Rule to check if static meshes exceed maximum triangle count thresholds.
 * High triangle counts can impact performance, especially on LOD0.
 * This rule helps maintain performance budgets and suggests LOD generation.
 */
class FStaticMeshTriangleCountRule : public IAssetCheckRule
{
public:
	FStaticMeshTriangleCountRule();
	virtual ~FStaticMeshTriangleCountRule() = default;

	// IAssetCheckRule interface
	virtual bool Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults) override;
	virtual FName GetRuleID() const override;
	virtual FText GetRuleDescription() const override;

private:
	/**
	 * Get triangle count for a specific LOD level
	 * @param StaticMesh The mesh to analyze
	 * @param LODIndex The LOD level to check (0 = highest detail)
	 * @return Number of triangles in the specified LOD
	 */
	int32 GetTriangleCount(const UStaticMesh* StaticMesh, int32 LODIndex = 0) const;

	/**
	 * Determine severity based on triangle count and thresholds
	 * @param TriangleCount Current triangle count
	 * @param WarningThreshold Warning threshold from settings
	 * @param ErrorThreshold Error threshold from settings
	 * @return Appropriate severity level
	 */
	EAssetIssueSeverity DetermineSeverity(int32 TriangleCount, int32 WarningThreshold, int32 ErrorThreshold) const;

	/**
	 * Note: Auto-fix methods removed - Triangle count reduction should be done in external 3D tools
	 * to preserve mesh quality, UVs, and shape integrity
	 */

	/**
	 * Generate detailed description of triangle count issues
	 * @param StaticMesh The mesh being analyzed
	 * @param TriangleCount Current triangle count
	 * @param BaseThreshold Base triangle count threshold
	 * @param PercentageThreshold Percentage threshold that was exceeded
	 * @param Severity Issue severity
	 * @return Formatted description string
	 */
	FString GenerateTriangleCountDescription(const UStaticMesh* StaticMesh, int32 TriangleCount, int32 BaseThreshold, float PercentageThreshold, EAssetIssueSeverity Severity) const;
}; 