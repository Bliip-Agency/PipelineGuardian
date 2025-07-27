#pragma once

#include "CoreMinimal.h"
#include "Analysis/IAssetCheckRule.h"
#include "Analysis/FAssetAnalysisResult.h"
#include "Engine/StaticMesh.h"

/**
 * Rule to detect static meshes missing collision geometry
 * Missing collision can cause physics issues, navigation problems, and gameplay bugs
 */
class FStaticMeshCollisionMissingRule : public IAssetCheckRule
{
public:
	FStaticMeshCollisionMissingRule();
	virtual ~FStaticMeshCollisionMissingRule() = default;

	// IAssetCheckRule interface
	virtual bool Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults) override;
	virtual FName GetRuleID() const override;
	virtual FText GetRuleDescription() const override;

private:
	/**
	 * Check if a static mesh has collision geometry
	 * @param StaticMesh The mesh to analyze
	 * @return True if collision is missing
	 */
	bool HasMissingCollision(const UStaticMesh* StaticMesh) const;

	/**
	 * Generate detailed description of missing collision issues
	 * @param StaticMesh The mesh being analyzed
	 * @param Severity Issue severity
	 * @return Formatted description string
	 */
	FString GenerateMissingCollisionDescription(const UStaticMesh* StaticMesh, EAssetIssueSeverity Severity) const;

	/**
	 * Generate collision geometry for the static mesh
	 * @param StaticMesh The mesh to fix
	 * @return True if collision was successfully generated
	 */
	bool GenerateCollision(UStaticMesh* StaticMesh) const;

	/**
	 * Check if automatic collision generation is safe for this mesh
	 * @param StaticMesh The mesh to analyze
	 * @return True if safe to auto-generate collision
	 */
	bool CanSafelyGenerateCollision(const UStaticMesh* StaticMesh) const;
}; 