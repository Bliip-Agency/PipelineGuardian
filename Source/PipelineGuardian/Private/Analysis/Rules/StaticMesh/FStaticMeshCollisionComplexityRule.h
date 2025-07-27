#pragma once

#include "CoreMinimal.h"
#include "Analysis/IAssetCheckRule.h"
#include "Analysis/FAssetAnalysisResult.h"
#include "Engine/StaticMesh.h"

/**
 * Rule to detect static meshes with overly complex collision geometry
 * Complex collision can cause performance issues, physics simulation problems, and memory overhead
 */
class FStaticMeshCollisionComplexityRule : public IAssetCheckRule
{
public:
	FStaticMeshCollisionComplexityRule();
	virtual ~FStaticMeshCollisionComplexityRule() = default;

	// IAssetCheckRule interface
	virtual bool Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults) override;
	virtual FName GetRuleID() const override;
	virtual FText GetRuleDescription() const override;

private:
	/**
	 * Check if a static mesh has overly complex collision
	 * @param StaticMesh The mesh to analyze
	 * @param OutPrimitiveCount Number of collision primitives found
	 * @param OutUseComplexAsSimple Whether UseComplexAsSimple is enabled
	 * @return True if collision is too complex
	 */
	bool HasComplexCollision(const UStaticMesh* StaticMesh, int32& OutPrimitiveCount, bool& OutUseComplexAsSimple) const;

	/**
	 * Generate detailed description of collision complexity issues
	 * @param StaticMesh The mesh being analyzed
	 * @param PrimitiveCount Number of collision primitives
	 * @param UseComplexAsSimple Whether UseComplexAsSimple is enabled
	 * @param Severity Issue severity
	 * @return Formatted description string
	 */
	FString GenerateCollisionComplexityDescription(const UStaticMesh* StaticMesh, int32 PrimitiveCount, bool UseComplexAsSimple, EAssetIssueSeverity Severity) const;

	/**
	 * Simplify collision geometry for the static mesh
	 * @param StaticMesh The mesh to fix
	 * @return True if collision was successfully simplified
	 */
	bool SimplifyCollision(UStaticMesh* StaticMesh) const;

	/**
	 * Check if automatic collision simplification is safe for this mesh
	 * @param StaticMesh The mesh to analyze
	 * @return True if safe to auto-simplify collision
	 */
	bool CanSafelySimplifyCollision(const UStaticMesh* StaticMesh) const;
}; 