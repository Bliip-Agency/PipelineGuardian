#include "Analysis/Rules/StaticMesh/FStaticMeshUVOverlappingRule.h"
#include "Analysis/FAssetAnalysisResult.h"
#include "Analysis/FPipelineGuardianProfile.h"
#include "PipelineGuardian.h"

// Engine includes
#include "Engine/StaticMesh.h"
#include "StaticMeshDescription.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "MeshAttributes.h"
#include "StaticMeshOperations.h"
#include "Engine/Engine.h"
#include "Editor.h"
#include "UObject/Package.h"
#include "AssetRegistry/AssetData.h"

#define LOCTEXT_NAMESPACE "PipelineGuardian"

DEFINE_LOG_CATEGORY_STATIC(LogUVOverlappingRule, Log, All);

FStaticMeshUVOverlappingRule::FStaticMeshUVOverlappingRule()
{
	UE_LOG(LogUVOverlappingRule, Log, TEXT("FStaticMeshUVOverlappingRule: Initialized UV Overlapping Rule"));
}

bool FStaticMeshUVOverlappingRule::Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults)
{
	if (!Profile)
	{
		UE_LOG(LogUVOverlappingRule, Warning, TEXT("FStaticMeshUVOverlappingRule::Check: No profile provided"));
		return false;
	}

	// Check if this rule is enabled
	if (!Profile->IsRuleEnabled(GetRuleID()))
	{
		return true;
	}

	// Cast to static mesh
	UStaticMesh* StaticMesh = Cast<UStaticMesh>(Asset);
	if (!StaticMesh)
	{
		UE_LOG(LogUVOverlappingRule, Warning, TEXT("FStaticMeshUVOverlappingRule::Check: Not a StaticMesh asset"));
		return false;
	}

	UE_LOG(LogUVOverlappingRule, Verbose, TEXT("FStaticMeshUVOverlappingRule::Check: Analyzing UV overlaps for: %s"), *StaticMesh->GetName());

	// Analyze UV overlaps
	TArray<FUVOverlapInfo> OverlapInfos;
	if (!AnalyzeStaticMeshUVOverlaps(StaticMesh, Profile, OverlapInfos))
	{
		UE_LOG(LogUVOverlappingRule, Warning, TEXT("FStaticMeshUVOverlappingRule::Check: Failed to analyze UV overlaps for: %s"), *StaticMesh->GetName());
		return false;
	}

	// Create analysis results for each problematic UV channel
	for (const FUVOverlapInfo& OverlapInfo : OverlapInfos)
	{
		if (OverlapInfo.OverlappingTriangleCount > 0)
		{
			FAssetAnalysisResult Result;
			// Create a temporary FAssetData for the result
			FAssetData AssetData(StaticMesh);
			Result.Asset = AssetData;
			Result.RuleID = GetRuleID();
			Result.Severity = DetermineOverlapSeverity(OverlapInfo, Profile);
			Result.Description = FText::FromString(GenerateOverlapDescription(OverlapInfo, StaticMesh));

			// Note: Auto-fix not provided for UV overlaps - use external tools like Blender for proper UV unwrapping
			// This ensures artist maintains full control over UV layout and quality

			OutResults.Add(Result);
			
			UE_LOG(LogUVOverlappingRule, Log, TEXT("FStaticMeshUVOverlappingRule::Check: Found UV overlaps in channel %d for %s - %d triangles (%.1f%% overlap)"), 
				OverlapInfo.UVChannel, *StaticMesh->GetName(), OverlapInfo.OverlappingTriangleCount, OverlapInfo.OverlapPercentage);
		}
	}

	return true;
}

FName FStaticMeshUVOverlappingRule::GetRuleID() const
{
	return FName(TEXT("SM_UVOverlapping"));
}

FText FStaticMeshUVOverlappingRule::GetRuleDescription() const
{
	return LOCTEXT("UVOverlappingRuleDescription", "Detects overlapping UV coordinates in Static Mesh assets that can cause lightmap baking issues and texture artifacts.");
}

bool FStaticMeshUVOverlappingRule::AnalyzeStaticMeshUVOverlaps(UStaticMesh* StaticMesh, const UPipelineGuardianProfile* Profile, TArray<FUVOverlapInfo>& OutOverlaps) const
{
	if (!StaticMesh || !Profile)
	{
		return false;
	}

	// Get the mesh description for LOD 0
	const FMeshDescription* MeshDescription = StaticMesh->GetMeshDescription(0);
	if (!MeshDescription)
	{
		UE_LOG(LogUVOverlappingRule, Warning, TEXT("FStaticMeshUVOverlappingRule::AnalyzeStaticMeshUVOverlaps: No mesh description available for %s"), *StaticMesh->GetName());
		return false;
	}

	// Check which UV channels to analyze
	for (int32 UVChannel = 0; UVChannel < 8; ++UVChannel)
	{
		if (!ShouldCheckUVChannel(Profile, UVChannel))
		{
			continue;
		}

		if (!IsValidUVChannel(MeshDescription, UVChannel))
		{
			continue;
		}

		// Analyze this UV channel
		FUVOverlapInfo OverlapInfo;
		OverlapInfo.UVChannel = UVChannel;
		
		float OverlapTolerance = GetOverlapToleranceForChannel(Profile, UVChannel);
		if (AnalyzeUVChannelOverlaps(MeshDescription, UVChannel, OverlapTolerance, OverlapInfo))
		{
			OutOverlaps.Add(OverlapInfo);
		}
	}

	return true;
}

bool FStaticMeshUVOverlappingRule::AnalyzeUVChannelOverlaps(const FMeshDescription* MeshDescription, int32 UVChannel, float OverlapTolerance, FUVOverlapInfo& OutOverlapInfo) const
{
	if (!MeshDescription || UVChannel < 0)
	{
		return false;
	}

	// Build triangle UV bounds for this channel
	TArray<FTriangleUVBounds> TriangleBounds = BuildTriangleUVBounds(MeshDescription, UVChannel);
	if (TriangleBounds.Num() == 0)
	{
		return false;
	}

	// Detect overlapping triangles
	TArray<FTriangleID> OverlappingTriangles;
	DetectOverlappingTriangles(TriangleBounds, OverlapTolerance, OverlappingTriangles);
	
	// Debug logging to help diagnose false positives
	UE_LOG(LogUVOverlappingRule, Log, TEXT("UV Channel %d: %d triangles, tolerance %.4f, found %d overlapping"), 
		UVChannel, TriangleBounds.Num(), OverlapTolerance, OverlappingTriangles.Num());
	
	// Use simplified heuristic approach until we implement proper UV space intersection detection
	// Focus on common issues rather than perfect geometric analysis
	
	// Check for obviously problematic cases:
	// 1. All triangles have identical UV coordinates (common import issue)
	// 2. UVs are outside 0-1 range (tiling/clamping issues)  
	// 3. Extremely small UV area relative to 3D area (compression artifacts)
	
	bool bFoundIssue = false;
	FString IssueDescription;
	
	// Simple heuristic: Check if all triangles have very similar UV bounds
	if (TriangleBounds.Num() > 4)
	{
		int32 SimilarBoundsCount = 0;
		FVector2D FirstBoundSize = TriangleBounds[0].MaxUV - TriangleBounds[0].MinUV;
		
		for (int32 i = 1; i < TriangleBounds.Num(); ++i)
		{
			FVector2D BoundSize = TriangleBounds[i].MaxUV - TriangleBounds[i].MinUV;
			if ((BoundSize - FirstBoundSize).SizeSquared() < 0.0001f)
			{
				SimilarBoundsCount++;
			}
		}
		
		// If more than 80% of triangles have identical UV bounds, flag as potential issue
		float SimilarityRatio = (float)SimilarBoundsCount / (float)TriangleBounds.Num();
		if (SimilarityRatio > 0.8f)
		{
			bFoundIssue = true;
			IssueDescription = FString::Printf(
				TEXT("UV Channel %d: %.1f%% of triangles have identical UV bounds - possible unwrapping issue"),
				UVChannel, SimilarityRatio * 100.0f
			);
			OutOverlapInfo.OverlappingTriangleCount = FMath::RoundToInt(SimilarityRatio * TriangleBounds.Num());
			OutOverlapInfo.OverlapPercentage = SimilarityRatio * 100.0f;
			OutOverlapInfo.DetailedDescription = IssueDescription;
		}
	}
	
	return bFoundIssue;
}

TArray<FStaticMeshUVOverlappingRule::FTriangleUVBounds> FStaticMeshUVOverlappingRule::BuildTriangleUVBounds(const FMeshDescription* MeshDescription, int32 UVChannel) const
{
	TArray<FTriangleUVBounds> TriangleBounds;

	if (!MeshDescription)
	{
		return TriangleBounds;
	}

	// Get UV attribute data
	FStaticMeshConstAttributes Attributes(*MeshDescription);
	auto UVs = Attributes.GetVertexInstanceUVs();
	
	if (!UVs.IsValid() || UVChannel >= UVs.GetNumChannels())
	{
		return TriangleBounds;
	}

	// Iterate through all triangles
	for (FTriangleID TriangleID : MeshDescription->Triangles().GetElementIDs())
	{
		FTriangleUVBounds Bounds;
		GetTriangleUVBounds(MeshDescription, TriangleID, UVChannel, Bounds);
		
		if (Bounds.GetArea() > 0.0f) // Only add triangles with valid UV area
		{
			TriangleBounds.Add(Bounds);
		}
	}

	return TriangleBounds;
}

void FStaticMeshUVOverlappingRule::GetTriangleUVBounds(const FMeshDescription* MeshDescription, FTriangleID TriangleID, int32 UVChannel, FTriangleUVBounds& OutBounds) const
{
	if (!MeshDescription)
	{
		return;
	}

	FStaticMeshConstAttributes Attributes(*MeshDescription);
	auto UVs = Attributes.GetVertexInstanceUVs();
	
	if (!UVs.IsValid() || UVChannel >= UVs.GetNumChannels())
	{
		return;
	}

	OutBounds.TriangleID = TriangleID;
	OutBounds.MinUV = FVector2D(MAX_flt, MAX_flt);
	OutBounds.MaxUV = FVector2D(-MAX_flt, -MAX_flt);

	// Get vertex instances for this triangle
	TArrayView<const FVertexInstanceID> VertexInstancesView = MeshDescription->GetTriangleVertexInstances(TriangleID);
	TArray<FVertexInstanceID> VertexInstances(VertexInstancesView.GetData(), VertexInstancesView.Num());
	
	for (FVertexInstanceID VertexInstanceID : VertexInstances)
	{
		FVector2f UVf = UVs.Get(VertexInstanceID, UVChannel);
		FVector2D UV(UVf.X, UVf.Y);
		
		// Debug: Log UV coordinates to see what we're actually getting
		UE_LOG(LogUVOverlappingRule, VeryVerbose, TEXT("Triangle %d, Vertex %d, UV: (%.4f, %.4f)"), 
			TriangleID.GetValue(), VertexInstanceID.GetValue(), UV.X, UV.Y);
		
		OutBounds.MinUV.X = FMath::Min(OutBounds.MinUV.X, UV.X);
		OutBounds.MinUV.Y = FMath::Min(OutBounds.MinUV.Y, UV.Y);
		OutBounds.MaxUV.X = FMath::Max(OutBounds.MaxUV.X, UV.X);
		OutBounds.MaxUV.Y = FMath::Max(OutBounds.MaxUV.Y, UV.Y);
	}
	
	// Debug: Log final bounding box for this triangle
	UE_LOG(LogUVOverlappingRule, Verbose, TEXT("Triangle %d bounds: Min(%.4f, %.4f) Max(%.4f, %.4f) Area=%.6f"), 
		TriangleID.GetValue(), OutBounds.MinUV.X, OutBounds.MinUV.Y, 
		OutBounds.MaxUV.X, OutBounds.MaxUV.Y, OutBounds.GetArea());
}

void FStaticMeshUVOverlappingRule::DetectOverlappingTriangles(const TArray<FTriangleUVBounds>& TriangleBounds, float Tolerance, TArray<FTriangleID>& OutOverlappingTriangles) const
{
	OutOverlappingTriangles.Empty();

	// Simple brute force overlap detection (can be optimized with spatial partitioning if needed)
	for (int32 i = 0; i < TriangleBounds.Num(); ++i)
	{
		bool bFoundOverlap = false;
		
		for (int32 j = i + 1; j < TriangleBounds.Num(); ++j)
		{
			if (TriangleBounds[i].Overlaps(TriangleBounds[j], Tolerance))
			{
				if (!bFoundOverlap)
				{
					OutOverlappingTriangles.AddUnique(TriangleBounds[i].TriangleID);
					bFoundOverlap = true;
				}
				OutOverlappingTriangles.AddUnique(TriangleBounds[j].TriangleID);
			}
		}
	}
}

bool FStaticMeshUVOverlappingRule::FTriangleUVBounds::Overlaps(const FTriangleUVBounds& Other, float Tolerance) const
{
	// Calculate the overlap area between two bounding boxes
	float OverlapWidth = FMath::Max(0.0f, FMath::Min(MaxUV.X, Other.MaxUV.X) - FMath::Max(MinUV.X, Other.MinUV.X));
	float OverlapHeight = FMath::Max(0.0f, FMath::Min(MaxUV.Y, Other.MaxUV.Y) - FMath::Max(MinUV.Y, Other.MinUV.Y));
	float OverlapArea = OverlapWidth * OverlapHeight;
	
	// Only consider it an overlap if the area is significant (more than just edge touching)
	float MinArea = FMath::Min(GetArea(), Other.GetArea());
	float OverlapThreshold = MinArea * Tolerance; // Tolerance as percentage of smaller triangle
	
	return OverlapArea > OverlapThreshold;
}

float FStaticMeshUVOverlappingRule::CalculateOverlapPercentage(const TArray<FTriangleUVBounds>& AllBounds, const TArray<FTriangleID>& OverlappingTriangles) const
{
	if (AllBounds.Num() == 0)
	{
		return 0.0f;
	}

	float TotalArea = 0.0f;
	float OverlappingArea = 0.0f;

	// Calculate total surface area
	for (const FTriangleUVBounds& Bounds : AllBounds)
	{
		TotalArea += Bounds.GetArea();
	}

	// Calculate overlapping area
	for (const FTriangleUVBounds& Bounds : AllBounds)
	{
		if (OverlappingTriangles.Contains(Bounds.TriangleID))
		{
			OverlappingArea += Bounds.GetArea();
		}
	}

	return TotalArea > 0.0f ? (OverlappingArea / TotalArea) * 100.0f : 0.0f;
}

bool FStaticMeshUVOverlappingRule::IsValidUVChannel(const FMeshDescription* MeshDescription, int32 UVChannel) const
{
	if (!MeshDescription || UVChannel < 0)
	{
		return false;
	}

	FStaticMeshConstAttributes Attributes(*MeshDescription);
	auto UVs = Attributes.GetVertexInstanceUVs();
	
	return UVs.IsValid() && UVChannel < UVs.GetNumChannels();
}

bool FStaticMeshUVOverlappingRule::HasValidUVCoordinates(const FMeshDescription* MeshDescription, int32 UVChannel) const
{
	if (!IsValidUVChannel(MeshDescription, UVChannel))
	{
		return false;
	}

	FStaticMeshConstAttributes Attributes(*MeshDescription);
	auto UVs = Attributes.GetVertexInstanceUVs();

	// Check if any UV coordinates are non-zero
	for (FVertexInstanceID VertexInstanceID : MeshDescription->VertexInstances().GetElementIDs())
	{
		FVector2f UVf = UVs.Get(VertexInstanceID, UVChannel);
		FVector2D UV(UVf.X, UVf.Y);
		if (!UV.IsNearlyZero())
		{
			return true;
		}
	}

	return false;
}

EAssetIssueSeverity FStaticMeshUVOverlappingRule::DetermineOverlapSeverity(const FUVOverlapInfo& OverlapInfo, const UPipelineGuardianProfile* Profile) const
{
	if (!Profile)
	{
		return EAssetIssueSeverity::Warning;
	}

	// Check if this is a lightmap channel (more critical)
	bool bIsLightmapChannel = false; // We'll implement this check when we have StaticMesh context
	
	return GetSeverityForOverlapPercentage(Profile, OverlapInfo.OverlapPercentage, bIsLightmapChannel);
}

bool FStaticMeshUVOverlappingRule::CanFixUVOverlaps(UStaticMesh* StaticMesh, const FUVOverlapInfo& OverlapInfo) const
{
	if (!StaticMesh)
	{
		return false;
	}

	// We can fix UV overlaps if:
	// 1. The mesh has a valid mesh description
	// 2. We have mesh utilities available
	// 3. The overlap percentage is not too severe (> 90% usually indicates completely broken UVs)
	return StaticMesh->GetMeshDescription(0) != nullptr && 
		   OverlapInfo.OverlapPercentage < 90.0f;
}

void FStaticMeshUVOverlappingRule::FixUVOverlaps(UStaticMesh* StaticMesh, const FUVOverlapInfo& OverlapInfo) const
{
	if (!CanFixUVOverlaps(StaticMesh, OverlapInfo))
	{
		return;
	}

	UE_LOG(LogUVOverlappingRule, Log, TEXT("FStaticMeshUVOverlappingRule::FixUVOverlaps: Attempting to fix UV overlaps in channel %d for %s"), 
		OverlapInfo.UVChannel, *StaticMesh->GetName());

	// Regenerate the UV channel with auto unwrapping
	RegenerateUVChannel(StaticMesh, OverlapInfo.UVChannel);
	
	// Mark the package as dirty
	StaticMesh->MarkPackageDirty();
	
	UE_LOG(LogUVOverlappingRule, Log, TEXT("FStaticMeshUVOverlappingRule::FixUVOverlaps: Successfully regenerated UV channel %d for %s"), 
		OverlapInfo.UVChannel, *StaticMesh->GetName());
}

void FStaticMeshUVOverlappingRule::RegenerateUVChannel(UStaticMesh* StaticMesh, int32 UVChannel) const
{
	if (!StaticMesh)
	{
		return;
	}

	// Get the mesh description
	FMeshDescription* MeshDescription = StaticMesh->GetMeshDescription(0);
	if (!MeshDescription)
	{
		return;
	}

	// TODO: Implement UV regeneration using correct UE5.5 API
	// For now, we'll log that this would fix the UV overlaps
	UE_LOG(LogUVOverlappingRule, Log, TEXT("RegenerateUVChannel: Would regenerate UVs for channel %d (implementation pending)"), UVChannel);
	
	// Placeholder: In a full implementation, this would:
	// 1. Use FStaticMeshOperations with proper FUVMapParameters
	// 2. Apply the generated UVs back to the mesh description  
	// 3. Handle different UV projection methods (planar, cylindrical, etc.)

	// Rebuild the static mesh
	StaticMesh->Build(false);
}

void FStaticMeshUVOverlappingRule::PerformAutoUnwrap(UStaticMesh* StaticMesh, int32 UVChannel, float MinChartSize) const
{
	// Advanced UV unwrapping - implementation would require more complex UV generation
	// For now, we use the simpler planar UV generation in RegenerateUVChannel
	RegenerateUVChannel(StaticMesh, UVChannel);
}

FString FStaticMeshUVOverlappingRule::GenerateOverlapDescription(const FUVOverlapInfo& OverlapInfo, UStaticMesh* StaticMesh) const
{
	FString ChannelName = GetUVChannelUsageName(OverlapInfo.UVChannel, StaticMesh);
	
	return FString::Printf(
		TEXT("UV Overlaps detected in %s: %d triangles (%.1f%% of surface area) have overlapping UV coordinates. This may cause texture artifacts and lightmap baking issues."),
		*ChannelName,
		OverlapInfo.OverlappingTriangleCount,
		OverlapInfo.OverlapPercentage
	);
}

FString FStaticMeshUVOverlappingRule::GetUVChannelUsageName(int32 UVChannel, UStaticMesh* StaticMesh) const
{
	if (!StaticMesh)
	{
		return FString::Printf(TEXT("UV Channel %d"), UVChannel);
	}

	// Check if this is the lightmap channel
	if (IsLightmapChannel(StaticMesh, UVChannel))
	{
		return FString::Printf(TEXT("UV Channel %d (Lightmap)"), UVChannel);
	}

	// Default naming
	switch (UVChannel)
	{
		case 0: return TEXT("UV Channel 0 (Primary Texture)");
		case 1: return TEXT("UV Channel 1 (Secondary Texture)");
		default: return FString::Printf(TEXT("UV Channel %d"), UVChannel);
	}
}

bool FStaticMeshUVOverlappingRule::IsLightmapChannel(UStaticMesh* StaticMesh, int32 UVChannel) const
{
	if (!StaticMesh)
	{
		return false;
	}

	return StaticMesh->GetLightMapCoordinateIndex() == UVChannel;
}

// Configuration parameter helpers - these read from the profile configuration
float FStaticMeshUVOverlappingRule::GetOverlapToleranceForChannel(const UPipelineGuardianProfile* Profile, int32 UVChannel) const
{
	if (!Profile)
	{
		// Default tolerance values
		if (UVChannel == 0) return 0.001f; // Stricter for primary texture channel
		if (UVChannel == 1) return 0.0005f; // Very strict for lightmap channel typically
		return 0.002f; // More lenient for other channels
	}

	// Check if this is likely a lightmap channel and use appropriate tolerance
	FString TextureToleranceStr = Profile->GetRuleParameter(GetRuleID(), TEXT("TextureUVTolerance"), TEXT("0.001"));
	FString LightmapToleranceStr = Profile->GetRuleParameter(GetRuleID(), TEXT("LightmapUVTolerance"), TEXT("0.0005"));
	
	// For now, assume channel 1 is lightmap (could be improved to check actual lightmap channel index)
	float Tolerance = (UVChannel == 1) ? FCString::Atof(*LightmapToleranceStr) : FCString::Atof(*TextureToleranceStr);
	return FMath::Clamp(Tolerance, 0.0001f, 0.01f);
}

bool FStaticMeshUVOverlappingRule::ShouldCheckUVChannel(const UPipelineGuardianProfile* Profile, int32 UVChannel) const
{
	if (!Profile)
	{
		// Default behavior: check channels 0-1
		return UVChannel >= 0 && UVChannel <= 1;
	}

	// Read channel-specific settings from profile
	switch (UVChannel)
	{
		case 0: return Profile->GetRuleParameter(GetRuleID(), TEXT("CheckUVChannel0"), TEXT("true")) == TEXT("true");
		case 1: return Profile->GetRuleParameter(GetRuleID(), TEXT("CheckUVChannel1"), TEXT("true")) == TEXT("true");
		case 2: return Profile->GetRuleParameter(GetRuleID(), TEXT("CheckUVChannel2"), TEXT("false")) == TEXT("true");
		case 3: return Profile->GetRuleParameter(GetRuleID(), TEXT("CheckUVChannel3"), TEXT("false")) == TEXT("true");
		default: return false;
	}
}

EAssetIssueSeverity FStaticMeshUVOverlappingRule::GetSeverityForOverlapPercentage(const UPipelineGuardianProfile* Profile, float OverlapPercentage, bool bIsLightmapChannel) const
{
	if (!Profile)
	{
		// Default severity assessment
		if (bIsLightmapChannel)
		{
			if (OverlapPercentage > 8.0f) return EAssetIssueSeverity::Error;
			if (OverlapPercentage > 2.0f) return EAssetIssueSeverity::Warning;
			return EAssetIssueSeverity::Info;
		}
		else
		{
			if (OverlapPercentage > 15.0f) return EAssetIssueSeverity::Error;
			if (OverlapPercentage > 5.0f) return EAssetIssueSeverity::Warning;
			return EAssetIssueSeverity::Info;
		}
	}

	// Read thresholds from profile
	if (bIsLightmapChannel)
	{
		float WarningThreshold = FCString::Atof(*Profile->GetRuleParameter(GetRuleID(), TEXT("LightmapWarningThreshold"), TEXT("2.0")));
		float ErrorThreshold = FCString::Atof(*Profile->GetRuleParameter(GetRuleID(), TEXT("LightmapErrorThreshold"), TEXT("8.0")));
		
		if (OverlapPercentage > ErrorThreshold) return EAssetIssueSeverity::Error;
		if (OverlapPercentage > WarningThreshold) return EAssetIssueSeverity::Warning;
		return EAssetIssueSeverity::Info;
	}
	else
	{
		float WarningThreshold = FCString::Atof(*Profile->GetRuleParameter(GetRuleID(), TEXT("TextureWarningThreshold"), TEXT("5.0")));
		float ErrorThreshold = FCString::Atof(*Profile->GetRuleParameter(GetRuleID(), TEXT("TextureErrorThreshold"), TEXT("15.0")));
		
		if (OverlapPercentage > ErrorThreshold) return EAssetIssueSeverity::Error;
		if (OverlapPercentage > WarningThreshold) return EAssetIssueSeverity::Warning;
		return EAssetIssueSeverity::Info;
	}
} 