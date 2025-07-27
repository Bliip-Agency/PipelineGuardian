#pragma once

#include "CoreMinimal.h"
#include "Analysis/IAssetCheckRule.h"
#include "Analysis/FAssetAnalysisResult.h"
#include "Engine/StaticMesh.h"

/**
 * Rule to detect degenerate faces (zero-area triangles) in static meshes
 * Degenerate faces can cause rendering artifacts, physics issues, and performance problems
 */
class FStaticMeshDegenerateFacesRule : public IAssetCheckRule
{
public:
	FStaticMeshDegenerateFacesRule();
	virtual ~FStaticMeshDegenerateFacesRule() = default;

	// IAssetCheckRule interface
	virtual bool Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults) override;
	virtual FName GetRuleID() const override;
	virtual FText GetRuleDescription() const override;

private:
	/**
	 * Check if a static mesh has degenerate faces
	 * @param StaticMesh The mesh to analyze
	 * @param OutDegenerateFaceCount Number of degenerate faces found
	 * @param OutTotalFaceCount Total number of faces in the mesh
	 * @return True if degenerate faces were found
	 */
	bool HasDegenerateFaces(const UStaticMesh* StaticMesh, int32& OutDegenerateFaceCount, int32& OutTotalFaceCount) const;



	/**
	 * Generate detailed description of degenerate faces issues
	 * @param StaticMesh The mesh being analyzed
	 * @param DegenerateFaceCount Number of degenerate faces found
	 * @param TotalFaceCount Total number of faces in the mesh
	 * @param Severity Issue severity
	 * @return Formatted description string
	 */
	FString GenerateDegenerateFacesDescription(const UStaticMesh* StaticMesh, int32 DegenerateFaceCount, int32 TotalFaceCount, EAssetIssueSeverity Severity) const;

	/**
	 * Remove degenerate faces from the static mesh
	 * @param StaticMesh The mesh to fix
	 * @return True if degenerate faces were successfully removed
	 */
	bool RemoveDegenerateFaces(UStaticMesh* StaticMesh) const;

	/**
	 * Check if automatic fixing is safe for this mesh
	 * @param StaticMesh The mesh to analyze
	 * @param DegenerateFaceCount Number of degenerate faces
	 * @param TotalFaceCount Total number of faces
	 * @return True if safe to auto-fix
	 */
	bool CanSafelyRemoveDegenerateFaces(const UStaticMesh* StaticMesh, int32 DegenerateFaceCount, int32 TotalFaceCount) const;
}; 