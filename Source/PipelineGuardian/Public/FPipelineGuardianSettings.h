// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Analysis/FPipelineGuardianProfile.h"
#include "Analysis/FAssetAnalysisResult.h"
#include "FPipelineGuardianSettings.generated.h" // Should be the last include

/** Strategy for choosing lightmap UV channel */
UENUM(BlueprintType)
enum class ELightmapUVChannelStrategy : uint8
{
	/** Automatically find the next available UV channel */
	NextAvailable		UMETA(DisplayName = "Next Available Channel"),
	
	/** Use a specific preferred channel, fall back to next available if occupied */
	PreferredChannel	UMETA(DisplayName = "Preferred Channel (with fallback)"),
	
	/** Always use UV channel 1 (legacy behavior) */
	ForceChannel1		UMETA(DisplayName = "Always Use Channel 1")
};

/**
 * Settings for the Pipeline Guardian plugin.
 * These settings will be saved in Saved/Config/Windows/EditorPerProjectUserSettings.ini (or platform equivalent).
 */
UCLASS(Config=Engine, DefaultConfig, meta=(DisplayName="Pipeline Guardian"))
class PIPELINEGUARDIAN_API UPipelineGuardianSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPipelineGuardianSettings();

	/** UDeveloperSettings Overrides */
	virtual FName GetCategoryName() const override;

	/** Master switch to enable or disable all Pipeline Guardian analysis. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Global Settings", meta = (ToolTip = "If unchecked, Pipeline Guardian will not perform any analysis."))
	bool bMasterSwitch_EnableAnalysis;

	/** Path to the currently active profile asset */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Profile Management", meta = (AllowedClasses = "PipelineGuardianProfile"))
	FSoftObjectPath ActiveProfilePath;

	/** List of available profile paths for quick switching */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Profile Management")
	TArray<FSoftObjectPath> AvailableProfiles;

	// ========================================
	// Static Mesh Rules - Quick Settings (these modify the active profile)
	// ========================================
	
	/** Enable/disable Static Mesh naming convention checks */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Naming", meta = (ToolTip = "Enable naming convention checks for Static Mesh assets"))
	bool bEnableStaticMeshNamingRule;

	/** Naming pattern for Static Mesh assets (supports * and ? wildcards) */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Naming", meta = (ToolTip = "Expected naming pattern for Static Mesh assets. Use * for any characters, ? for single character"))
	FString StaticMeshNamingPattern;

	/** Enable/disable Static Mesh LOD missing checks */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|LOD Count", meta = (ToolTip = "Enable checks for missing LOD levels on Static Mesh assets"))
	bool bEnableStaticMeshLODRule;

	/** Minimum required number of LODs for Static Mesh assets */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|LOD Count", meta = (ToolTip = "Minimum number of LOD levels required for Static Mesh assets", ClampMin = "1", ClampMax = "8"))
	int32 MinRequiredLODs;

	/** Enable/disable Static Mesh LOD polygon reduction checks */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|LOD Quality", meta = (ToolTip = "Enable checks for insufficient polygon reduction between LOD levels"))
	bool bEnableStaticMeshLODPolyReductionRule;

	/** Minimum required polygon reduction percentage between LOD levels */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|LOD Quality", meta = (ToolTip = "Minimum percentage of polygon reduction required between consecutive LOD levels", ClampMin = "10.0", ClampMax = "90.0"))
	float MinLODReductionPercentage;

	/** Warning threshold for polygon reduction percentage */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|LOD Quality", meta = (ToolTip = "Polygon reduction percentage below which a warning is issued", ClampMin = "5.0", ClampMax = "50.0"))
	float LODReductionWarningThreshold;

	/** Error threshold for polygon reduction percentage */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|LOD Quality", meta = (ToolTip = "Polygon reduction percentage below which an error is issued", ClampMin = "0.0", ClampMax = "30.0"))
	float LODReductionErrorThreshold;

	/** Follow LOD quality settings when creating missing LODs */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|LOD Creation", meta = (ToolTip = "When enabled, newly created LODs will follow the minimum LOD reduction percentage setting"))
	bool bFollowLODQualitySettingsWhenCreating;

	/** Default reduction percentages for each LOD level when auto-creating LODs */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|LOD Creation", meta = (ToolTip = "Default reduction percentages for LOD1, LOD2, LOD3, etc. when auto-creating LODs. If empty, uses MinLODReductionPercentage progressively."))
	TArray<float> DefaultLODReductionPercentages;

	// ========================================
	// Static Mesh Rules - Lightmap UV Settings
	// ========================================
	
	/** Enable/disable Static Mesh lightmap UV missing checks */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Lightmap UV", meta = (ToolTip = "Enable checks for missing or invalid lightmap UV configuration on Static Mesh assets"))
	bool bEnableStaticMeshLightmapUVRule;

	/** Severity level for lightmap UV issues */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Lightmap UV", meta = (ToolTip = "Severity level for lightmap UV configuration issues"))
	EAssetIssueSeverity LightmapUVIssueSeverity;

	/** Require valid UV coordinates (not just existence of UV channel) */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Lightmap UV", meta = (ToolTip = "When enabled, validates that lightmap UV channels contain valid, non-overlapping coordinates"))
	bool bRequireValidLightmapUVs;

	/** Allow automatic fixing of lightmap UV issues */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Lightmap UV", meta = (ToolTip = "Allow Pipeline Guardian to automatically fix lightmap UV issues by enabling bGenerateLightmapUVs"))
	bool bAllowLightmapUVAutoFix;

	/** Lightmap UV channel assignment strategy */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Lightmap UV", meta = (ToolTip = "How to choose UV channel for lightmap generation"))
	ELightmapUVChannelStrategy LightmapUVChannelStrategy;

	/** Preferred destination UV channel for generated lightmap UVs (when using 'Preferred Channel' strategy) */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Lightmap UV", meta = (ToolTip = "Preferred UV channel for lightmap generation. Will fall back to next available if occupied.", ClampMin = "1", ClampMax = "7", EditCondition = "LightmapUVChannelStrategy == ELightmapUVChannelStrategy::PreferredChannel"))
	int32 PreferredLightmapUVChannel;

	// ========================================
	// Static Mesh Rules - UV Overlapping
	// ========================================

	/** Enable/disable Static Mesh UV overlapping checks */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|UV Overlapping", meta = (ToolTip = "Enable checks for overlapping UV coordinates that can cause texture artifacts and lightmap baking issues"))
	bool bEnableStaticMeshUVOverlappingRule;

	/** Severity level for UV overlapping issues */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|UV Overlapping", meta = (ToolTip = "Severity level for UV overlapping configuration issues"))
	EAssetIssueSeverity UVOverlappingIssueSeverity;

	/** Check UV Channel 0 (Primary Texture) for overlaps */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|UV Overlapping", meta = (ToolTip = "Check UV Channel 0 (typically used for primary texture mapping) for overlapping coordinates"))
	bool bCheckUVChannel0;

	/** Check UV Channel 1 (Secondary/Lightmap) for overlaps */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|UV Overlapping", meta = (ToolTip = "Check UV Channel 1 (typically used for lightmaps or secondary textures) for overlapping coordinates"))
	bool bCheckUVChannel1;

	/** Check UV Channel 2 for overlaps */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|UV Overlapping", meta = (ToolTip = "Check UV Channel 2 for overlapping coordinates"))
	bool bCheckUVChannel2;

	/** Check UV Channel 3 for overlaps */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|UV Overlapping", meta = (ToolTip = "Check UV Channel 3 for overlapping coordinates"))
	bool bCheckUVChannel3;

	/** Overlap tolerance for texture UV channels */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|UV Overlapping", meta = (ToolTip = "Tolerance for detecting UV overlaps in texture channels (smaller values = more strict)", ClampMin = "0.0001", ClampMax = "0.01"))
	float TextureUVOverlapTolerance;

	/** Overlap tolerance for lightmap UV channels */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|UV Overlapping", meta = (ToolTip = "Tolerance for detecting UV overlaps in lightmap channels (smaller values = more strict)", ClampMin = "0.0001", ClampMax = "0.01"))
	float LightmapUVOverlapTolerance;

	/** Overlap percentage threshold for warnings on texture channels */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|UV Overlapping", meta = (ToolTip = "Percentage of overlapping surface area that triggers a warning for texture channels", ClampMin = "0.1", ClampMax = "50.0"))
	float TextureUVOverlapWarningThreshold;

	/** Overlap percentage threshold for errors on texture channels */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|UV Overlapping", meta = (ToolTip = "Percentage of overlapping surface area that triggers an error for texture channels", ClampMin = "1.0", ClampMax = "90.0"))
	float TextureUVOverlapErrorThreshold;

	/** Overlap percentage threshold for warnings on lightmap channels */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|UV Overlapping", meta = (ToolTip = "Percentage of overlapping surface area that triggers a warning for lightmap channels", ClampMin = "0.1", ClampMax = "25.0"))
	float LightmapUVOverlapWarningThreshold;

	/** Overlap percentage threshold for errors on lightmap channels */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|UV Overlapping", meta = (ToolTip = "Percentage of overlapping surface area that triggers an error for lightmap channels", ClampMin = "0.5", ClampMax = "50.0"))
	float LightmapUVOverlapErrorThreshold;

	/** Note: Auto-fix removed - UV overlapping should be fixed in external tools like Blender for best quality */
	// bool bAllowUVOverlapAutoFix; // Removed - use external UV tools instead

	// --- Triangle Count Rule Settings ---

	/** Enable Triangle Count Rule to check for performance-impacting high polygon meshes */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Triangle Count", meta = (ToolTip = "Enable checking for static meshes with excessive triangle counts that may impact performance"))
	bool bEnableStaticMeshTriangleCountRule;

	/** Severity level for triangle count issues */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Triangle Count", meta = (ToolTip = "Severity level assigned to triangle count violations"))
	EAssetIssueSeverity TriangleCountIssueSeverity;

	/** Base triangle count threshold (fixed value) */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Triangle Count", meta = (ToolTip = "Base triangle count threshold. Meshes exceeding this by the warning/error percentages will trigger issues. Note: Triangle count optimization should be done in external 3D tools to preserve mesh quality.", ClampMin = "100", ClampMax = "1000000"))
	int32 TriangleCountBaseThreshold;

	/** Warning threshold percentage above base triangle count */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Triangle Count", meta = (ToolTip = "Warning when mesh exceeds base threshold by this percentage. Use external tools like Blender for mesh optimization to preserve UVs and shape integrity.", ClampMin = "10.0", ClampMax = "200.0"))
	float TriangleCountWarningPercentage;

	/** Error threshold percentage above base triangle count */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Triangle Count", meta = (ToolTip = "Error when mesh exceeds base threshold by this percentage. Automatic mesh reduction can damage UVs and mesh quality - use external 3D tools instead.", ClampMin = "20.0", ClampMax = "500.0"))
	float TriangleCountErrorPercentage;

	/** Note: Auto-fix removed - Triangle count reduction should be done in external 3D tools like Blender for best quality */
	// bool bAllowTriangleCountAutoFix; // Removed - use external mesh optimization tools instead
	// float PerformanceLODReductionTarget; // Removed - use external mesh optimization tools instead

	// --- Degenerate Faces Rule Settings ---
	/** Enable Degenerate Faces Rule to check for zero-area triangles */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Degenerate Faces", meta = (ToolTip = "Enable checking for degenerate faces (zero-area triangles) that can cause rendering artifacts and performance issues"))
	bool bEnableStaticMeshDegenerateFacesRule;

	/** Severity level for degenerate faces issues */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Degenerate Faces", meta = (ToolTip = "Severity level assigned to degenerate faces violations"))
	EAssetIssueSeverity DegenerateFacesIssueSeverity;

	/** Warning threshold percentage of degenerate faces */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Degenerate Faces", meta = (ToolTip = "Warning when percentage of degenerate faces exceeds this threshold", ClampMin = "0.1", ClampMax = "10.0"))
	float DegenerateFacesWarningThreshold;

	/** Error threshold percentage of degenerate faces */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Degenerate Faces", meta = (ToolTip = "Error when percentage of degenerate faces exceeds this threshold", ClampMin = "1.0", ClampMax = "25.0"))
	float DegenerateFacesErrorThreshold;

	/** Allow automatic removal of degenerate faces */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Degenerate Faces", meta = (ToolTip = "Allow Pipeline Guardian to automatically remove degenerate faces when safe to do so"))
	bool bAllowDegenerateFacesAutoFix;

	// --- Collision Missing Rule Settings ---
	/** Enable Collision Missing Rule to check for static meshes without collision geometry */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Collision Missing", meta = (ToolTip = "Enable checking for static meshes that are missing collision geometry"))
	bool bEnableStaticMeshCollisionMissingRule;

	/** Severity level for missing collision issues */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Collision Missing", meta = (ToolTip = "Severity level assigned to missing collision violations"))
	EAssetIssueSeverity CollisionMissingIssueSeverity;

	/** Allow automatic generation of collision geometry */
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Collision Missing", meta = (ToolTip = "Allow Pipeline Guardian to automatically generate collision geometry when safe to do so"))
	bool bAllowCollisionMissingAutoFix;

	// --- Collision Complexity Rule Settings ---
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Collision Complexity", meta = (ToolTip = "Enable checking for static meshes with overly complex collision geometry"))
	bool bEnableStaticMeshCollisionComplexityRule;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Collision Complexity", meta = (ToolTip = "Severity level assigned to collision complexity violations"))
	EAssetIssueSeverity CollisionComplexityIssueSeverity;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Collision Complexity", meta = (ToolTip = "Warning when collision has more than this many primitives", ClampMin = "5", ClampMax = "100"))
	int32 CollisionComplexityWarningThreshold;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Collision Complexity", meta = (ToolTip = "Error when collision has more than this many primitives", ClampMin = "10", ClampMax = "200"))
	int32 CollisionComplexityErrorThreshold;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Collision Complexity", meta = (ToolTip = "When enabled, meshes with UseComplexAsSimple enabled will be treated as errors"))
	bool bTreatUseComplexAsSimpleAsError;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Collision Complexity", meta = (ToolTip = "Allow Pipeline Guardian to automatically simplify collision geometry when safe to do so"))
	bool bAllowCollisionComplexityAutoFix;

	// --- Nanite Suitability Rule Settings ---
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Nanite Suitability", meta = (ToolTip = "Enable checking for static meshes that should use Nanite based on polygon count"))
	bool bEnableStaticMeshNaniteSuitabilityRule;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Nanite Suitability", meta = (ToolTip = "Severity level assigned to Nanite suitability violations"))
	EAssetIssueSeverity NaniteSuitabilityIssueSeverity;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Nanite Suitability", meta = (ToolTip = "Meshes with more than this many triangles should have Nanite enabled", ClampMin = "1000", ClampMax = "100000"))
	int32 NaniteSuitabilityThreshold;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Nanite Suitability", meta = (ToolTip = "Meshes with less than this many triangles should have Nanite disabled for performance", ClampMin = "100", ClampMax = "10000"))
	int32 NaniteDisableThreshold;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Nanite Suitability", meta = (ToolTip = "Allow Pipeline Guardian to automatically enable/disable Nanite based on polygon count"))
	bool bAllowNaniteSuitabilityAutoFix;

	// --- Material Slot Management Rule Settings ---
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Material Slots", meta = (ToolTip = "Enable checking for static meshes with material slot issues (count and empty slots)"))
	bool bEnableStaticMeshMaterialSlotRule;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Material Slots", meta = (ToolTip = "Severity level assigned to material slot violations"))
	EAssetIssueSeverity MaterialSlotIssueSeverity;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Material Slots", meta = (ToolTip = "Warning when static mesh has more than this many material slots", ClampMin = "2", ClampMax = "20"))
	int32 MaterialSlotWarningThreshold;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Material Slots", meta = (ToolTip = "Error when static mesh has more than this many material slots", ClampMin = "3", ClampMax = "30"))
	int32 MaterialSlotErrorThreshold;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Material Slots", meta = (ToolTip = "Allow Pipeline Guardian to automatically remove empty material slots (does not reduce total slot count)"))
	bool bAllowMaterialSlotAutoFix;

	// --- Vertex Color Missing Rule Settings ---
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Vertex Colors", meta = (ToolTip = "Enable checking for static meshes that are missing required vertex color channels"))
	bool bEnableStaticMeshVertexColorMissingRule;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Vertex Colors", meta = (ToolTip = "Severity level assigned to missing vertex color violations"))
	EAssetIssueSeverity VertexColorMissingIssueSeverity;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Vertex Colors", meta = (ToolTip = "Require vertex colors for meshes with more than this many triangles", ClampMin = "100", ClampMax = "10000"))
	int32 VertexColorRequiredThreshold;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Vertex Colors", meta = (ToolTip = "Allow Pipeline Guardian to automatically generate vertex colors when missing (disabled in UE 5.5 due to API limitations)"))
	bool bAllowVertexColorMissingAutoFix;

	// --- Enhanced Vertex Color Analysis Settings ---
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Vertex Colors", meta = (ToolTip = "Enable checking for unused vertex color channels that bloat data"))
	bool bEnableVertexColorUnusedChannelRule;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Vertex Colors", meta = (ToolTip = "Severity level assigned to unused vertex color channel violations"))
	EAssetIssueSeverity VertexColorUnusedChannelIssueSeverity;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Vertex Colors", meta = (ToolTip = "Check for specific vertex color channels (e.g., mask channels) that should be present"))
	bool bEnableVertexColorChannelValidation;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Vertex Colors", meta = (ToolTip = "Required vertex color channels (comma-separated, e.g., 'Mask,Detail')"))
	FString RequiredVertexColorChannels;

	// --- Transform Pivot Rule Settings ---
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Transform Pivot", meta = (ToolTip = "Enable checking for static meshes with problematic transform pivots"))
	bool bEnableStaticMeshTransformPivotRule;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Transform Pivot", meta = (ToolTip = "Severity level assigned to transform pivot violations"))
	EAssetIssueSeverity TransformPivotIssueSeverity;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Transform Pivot", meta = (ToolTip = "Warning when pivot is more than this distance from origin", ClampMin = "1.0", ClampMax = "1000.0"))
	float TransformPivotWarningDistance;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Transform Pivot", meta = (ToolTip = "Error when pivot is more than this distance from origin", ClampMin = "5.0", ClampMax = "5000.0"))
	float TransformPivotErrorDistance;
	// Note: Transform pivot auto-fix removed - use external DCC tools for pivot adjustments

	// --- Enhanced Transform Analysis Settings ---
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Transform Pivot", meta = (ToolTip = "Enable asset-type specific pivot validation (e.g., different rules for characters vs props)"))
	bool bEnableAssetTypeSpecificPivotRules;

	// --- Scaling Rule Settings ---
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Scaling", meta = (ToolTip = "Enable checking for non-uniform scaling"))
	bool bEnableNonUniformScaleDetection;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Scaling", meta = (ToolTip = "Severity level assigned to non-uniform scaling violations"))
	EAssetIssueSeverity NonUniformScaleIssueSeverity;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Scaling", meta = (ToolTip = "Warning ratio threshold for non-uniform scaling", ClampMin = "1.1", ClampMax = "10.0"))
	float NonUniformScaleWarningRatio;
	
	// --- Zero Scale Detection Settings ---
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Scaling", meta = (ToolTip = "Enable checking for zero scale components that can cause issues"))
	bool bEnableZeroScaleDetection;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Scaling", meta = (ToolTip = "Severity level assigned to zero scale violations"))
	EAssetIssueSeverity ZeroScaleIssueSeverity;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Scaling", meta = (ToolTip = "Minimum scale threshold before considering it zero", ClampMin = "0.001", ClampMax = "0.1"))
	float ZeroScaleThreshold;

	// --- Lightmap Resolution Rule Settings ---
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Lightmap Resolution", meta = (ToolTip = "Enable checking for static meshes with inappropriate lightmap resolution. Lightmap resolution is calculated as 2^value (e.g., value 4 = 16x16, value 6 = 64x64, value 8 = 256x256)"))
	bool bEnableStaticMeshLightmapResolutionRule;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Lightmap Resolution", meta = (ToolTip = "Severity level assigned to lightmap resolution violations"))
	EAssetIssueSeverity LightmapResolutionIssueSeverity;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Lightmap Resolution", meta = (ToolTip = "Minimum lightmap resolution (power of 2). Value 4 = 16x16, Value 6 = 64x64, Value 8 = 256x256", ClampMin = "1", ClampMax = "16"))
	int32 LightmapResolutionMin;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Lightmap Resolution", meta = (ToolTip = "Maximum lightmap resolution (power of 2). Value 4 = 16x16, Value 6 = 64x64, Value 8 = 256x256", ClampMin = "8", ClampMax = "32"))
	int32 LightmapResolutionMax;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Lightmap Resolution", meta = (ToolTip = "Allow Pipeline Guardian to automatically set optimal lightmap resolution"))
	bool bAllowLightmapResolutionAutoFix;

	// --- Socket Naming Rule Settings ---
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Socket Naming", meta = (ToolTip = "Enable checking for static meshes with improper socket naming conventions"))
	bool bEnableStaticMeshSocketNamingRule;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Socket Naming", meta = (ToolTip = "Severity level assigned to socket naming violations"))
	EAssetIssueSeverity SocketNamingIssueSeverity;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Socket Naming", meta = (ToolTip = "Socket names must start with this prefix (empty = no prefix required)"))
	FString SocketNamingPrefix;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Socket Naming", meta = (ToolTip = "Warning when socket transform is more than this distance from mesh bounds", ClampMin = "10.0", ClampMax = "1000.0"))
	float SocketTransformWarningDistance;
	UPROPERTY(Config, EditAnywhere, Category = "Static Mesh Rules|Socket Naming", meta = (ToolTip = "Allow Pipeline Guardian to automatically fix socket naming and positioning"))
	bool bAllowSocketNamingAutoFix;

	/**
	 * Gets the currently active profile. Loads it if not already loaded.
	 * @return Pointer to the active profile, or nullptr if none is set or failed to load.
	 */
	UFUNCTION(BlueprintCallable, Category = "Profile Management")
	UPipelineGuardianProfile* GetActiveProfile() const;

	/**
	 * Sets the active profile by path.
	 * @param ProfilePath The soft object path to the profile asset.
	 * @return True if the profile was successfully set, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Profile Management")
	bool SetActiveProfile(const FSoftObjectPath& ProfilePath);

	/**
	 * Creates a new profile with the given name and sets it as active.
	 * @param ProfileName The name for the new profile.
	 * @param Description Optional description for the profile.
	 * @return Pointer to the newly created profile, or nullptr if creation failed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Profile Management")
	UPipelineGuardianProfile* CreateNewProfile(const FString& ProfileName, const FString& Description = TEXT(""));

	/**
	 * Exports the active profile to a JSON file.
	 * @param FilePath The file path to export to.
	 * @return True if export was successful, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Profile Management")
	bool ExportActiveProfileToFile(const FString& FilePath);

	/**
	 * Imports a profile from a JSON file and optionally sets it as active.
	 * @param FilePath The file path to import from.
	 * @param bSetAsActive Whether to set the imported profile as the active one.
	 * @return Pointer to the imported profile, or nullptr if import failed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Profile Management")
	UPipelineGuardianProfile* ImportProfileFromFile(const FString& FilePath, bool bSetAsActive = true);

private:
	/** Cached reference to the currently loaded active profile */
	UPROPERTY(Transient)
	mutable TObjectPtr<UPipelineGuardianProfile> CachedActiveProfile;

	/** Syncs the quick settings to the active profile */
	void SyncQuickSettingsToProfile();
}; 