#pragma once

#include "CoreMinimal.h"
#include "Analysis/IAssetCheckRule.h"
#include "Engine/StaticMesh.h"

// Forward declarations
enum class EAssetIssueSeverity : uint8;
class UPipelineGuardianProfile;
struct FAssetAnalysisResult;
struct FVertexInstanceID;
struct FTriangleID;

/**
 * UV Overlapping Detection and Analysis Rule for Static Meshes
 * 
 * This rule detects overlapping UV coordinates that can cause issues with:
 * - Lightmap baking (overlaps in lightmap UV channel)
 * - Texture stretching and distortion 
 * - UV seam artifacts
 * - Baking quality issues
 * 
 * Features:
 * - Multi-channel UV analysis (UV0-UV7)
 * - Configurable overlap tolerance
 * - Triangle-level overlap detection
 * - Automatic unwrapping fix capabilities
 * - Lightmap-specific validation
 */
class FStaticMeshUVOverlappingRule : public IAssetCheckRule
{
public:
	FStaticMeshUVOverlappingRule();
	virtual ~FStaticMeshUVOverlappingRule() = default;

	// IAssetCheckRule interface
	virtual bool Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults) override;
	virtual FName GetRuleID() const override;
	virtual FText GetRuleDescription() const override;

private:
	/**
	 * UV Overlap Detection Result
	 */
	struct FUVOverlapInfo
	{
		int32 UVChannel;
		int32 OverlappingTriangleCount;
		float OverlapPercentage;
		TArray<FTriangleID> OverlappingTriangles;
		FString DetailedDescription;
		
		FUVOverlapInfo()
			: UVChannel(0)
			, OverlappingTriangleCount(0)
			, OverlapPercentage(0.0f)
		{}
	};

	/**
	 * Triangle UV bounds for overlap testing
	 */
	struct FTriangleUVBounds
	{
		FVector2D MinUV;
		FVector2D MaxUV;
		FTriangleID TriangleID;
		
		FTriangleUVBounds() : MinUV(FVector2D::ZeroVector), MaxUV(FVector2D::ZeroVector) {}
		FTriangleUVBounds(const FVector2D& InMinUV, const FVector2D& InMaxUV, FTriangleID InTriangleID)
			: MinUV(InMinUV), MaxUV(InMaxUV), TriangleID(InTriangleID) {}
			
		bool Overlaps(const FTriangleUVBounds& Other, float Tolerance = 0.001f) const;
		float GetArea() const { return (MaxUV.X - MinUV.X) * (MaxUV.Y - MinUV.Y); }
	};

	// Core analysis functions
	bool AnalyzeStaticMeshUVOverlaps(UStaticMesh* StaticMesh, const UPipelineGuardianProfile* Profile, TArray<FUVOverlapInfo>& OutOverlaps) const;
	bool AnalyzeUVChannelOverlaps(const FMeshDescription* MeshDescription, int32 UVChannel, float OverlapTolerance, FUVOverlapInfo& OutOverlapInfo) const;
	void GetTriangleUVBounds(const FMeshDescription* MeshDescription, FTriangleID TriangleID, int32 UVChannel, FTriangleUVBounds& OutBounds) const;
	
	// UV validation utilities
	bool IsValidUVChannel(const FMeshDescription* MeshDescription, int32 UVChannel) const;
	bool HasValidUVCoordinates(const FMeshDescription* MeshDescription, int32 UVChannel) const;
	float CalculateUVUtilization(const FMeshDescription* MeshDescription, int32 UVChannel) const;
	
	// Overlap detection algorithms
	TArray<FTriangleUVBounds> BuildTriangleUVBounds(const FMeshDescription* MeshDescription, int32 UVChannel) const;
	void DetectOverlappingTriangles(const TArray<FTriangleUVBounds>& TriangleBounds, float Tolerance, TArray<FTriangleID>& OutOverlappingTriangles) const;
	float CalculateOverlapPercentage(const TArray<FTriangleUVBounds>& AllBounds, const TArray<FTriangleID>& OverlappingTriangles) const;
	
	// Severity assessment
	EAssetIssueSeverity DetermineOverlapSeverity(const FUVOverlapInfo& OverlapInfo, const UPipelineGuardianProfile* Profile) const;
	bool IsLightmapChannel(UStaticMesh* StaticMesh, int32 UVChannel) const;
	
	// Fix functionality
	bool CanFixUVOverlaps(UStaticMesh* StaticMesh, const FUVOverlapInfo& OverlapInfo) const;
	void FixUVOverlaps(UStaticMesh* StaticMesh, const FUVOverlapInfo& OverlapInfo) const;
	void RegenerateUVChannel(UStaticMesh* StaticMesh, int32 UVChannel) const;
	void PerformAutoUnwrap(UStaticMesh* StaticMesh, int32 UVChannel, float MinChartSize = 0.01f) const;
	
	// Helper functions for detailed reporting
	FString GenerateOverlapDescription(const FUVOverlapInfo& OverlapInfo, UStaticMesh* StaticMesh) const;
	FString GetUVChannelUsageName(int32 UVChannel, UStaticMesh* StaticMesh) const;
	
	// Configuration parameter helpers
	float GetOverlapToleranceForChannel(const UPipelineGuardianProfile* Profile, int32 UVChannel) const;
	bool ShouldCheckUVChannel(const UPipelineGuardianProfile* Profile, int32 UVChannel) const;
	EAssetIssueSeverity GetSeverityForOverlapPercentage(const UPipelineGuardianProfile* Profile, float OverlapPercentage, bool bIsLightmapChannel) const;
}; 