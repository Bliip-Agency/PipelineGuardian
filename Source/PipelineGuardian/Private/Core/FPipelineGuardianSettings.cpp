// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPipelineGuardianSettings.h"
#include "Analysis/FPipelineGuardianProfile.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "PipelineGuardian.h"

UPipelineGuardianSettings::UPipelineGuardianSettings()
	: bMasterSwitch_EnableAnalysis(true) // Default to enabled
	, bEnableStaticMeshNamingRule(true) // Default to enabled
	, StaticMeshNamingPattern(TEXT("SM_*")) // Default pattern
	, bEnableStaticMeshLODRule(true) // Default to enabled
	, MinRequiredLODs(3) // Default to 3 LODs
	, bEnableStaticMeshLODPolyReductionRule(true) // Default to enabled
	, MinLODReductionPercentage(30.0f) // Default to 30% reduction
	, LODReductionWarningThreshold(20.0f)
	, LODReductionErrorThreshold(10.0f)
	, bFollowLODQualitySettingsWhenCreating(true) // Default to enabled
	, bEnableStaticMeshLightmapUVRule(true) // Default to enabled
	, LightmapUVIssueSeverity(EAssetIssueSeverity::Warning) // Default to Warning
	, bRequireValidLightmapUVs(true) // Default to enabled
	, bAllowLightmapUVAutoFix(true) // Default to enabled
	, LightmapUVChannelStrategy(ELightmapUVChannelStrategy::NextAvailable) // Default to smart auto-detection
	, PreferredLightmapUVChannel(1) // Default to UV channel 1 when using preferred strategy
	, bEnableStaticMeshUVOverlappingRule(true) // Default to enabled
	, UVOverlappingIssueSeverity(EAssetIssueSeverity::Warning) // Default to Warning
	, bCheckUVChannel0(true) // Check primary texture channel
	, bCheckUVChannel1(true) // Check secondary/lightmap channel
	, bCheckUVChannel2(false) // Optional additional channels
	, bCheckUVChannel3(false)
	, TextureUVOverlapTolerance(0.01f) // Reasonable tolerance for textures (1% area)
	, LightmapUVOverlapTolerance(0.005f) // Stricter for lightmaps (0.5% area)
	, TextureUVOverlapWarningThreshold(5.0f) // 5% overlap triggers warning
	, TextureUVOverlapErrorThreshold(15.0f) // 15% overlap triggers error
	, LightmapUVOverlapWarningThreshold(2.0f) // 2% overlap triggers warning for lightmaps
	, LightmapUVOverlapErrorThreshold(8.0f) // 8% overlap triggers error for lightmaps
	// bAllowUVOverlapAutoFix removed - use external UV tools instead

	// Triangle Count Rule settings
	, bEnableStaticMeshTriangleCountRule(true)
	, TriangleCountIssueSeverity(EAssetIssueSeverity::Warning)
	, TriangleCountBaseThreshold(50000)     // 50K triangles - base threshold
	, TriangleCountWarningPercentage(20.0f) // 20% above base - warning
	, TriangleCountErrorPercentage(50.0f)   // 50% above base - error
	// Note: Auto-fix properties removed - use external 3D tools for mesh optimization
	, bEnableStaticMeshDegenerateFacesRule(true)
	, DegenerateFacesIssueSeverity(EAssetIssueSeverity::Warning)
	, DegenerateFacesWarningThreshold(1.0f)  // 1% degenerate faces - warning
	, DegenerateFacesErrorThreshold(5.0f)    // 5% degenerate faces - error
	, bAllowDegenerateFacesAutoFix(true)     // Allow automatic removal when safe
	, bEnableStaticMeshCollisionMissingRule(true)
	, CollisionMissingIssueSeverity(EAssetIssueSeverity::Error) // Missing collision is critical
	, bAllowCollisionMissingAutoFix(true)    // Allow automatic generation when safe
	, bEnableStaticMeshCollisionComplexityRule(true)
	, CollisionComplexityIssueSeverity(EAssetIssueSeverity::Warning)
	, CollisionComplexityWarningThreshold(15) // 15 primitives - warning
	, CollisionComplexityErrorThreshold(25)   // 25 primitives - error
	, bTreatUseComplexAsSimpleAsError(true)   // UseComplexAsSimple is often problematic
	, bAllowCollisionComplexityAutoFix(true)  // Allow automatic simplification when safe
	, bEnableStaticMeshNaniteSuitabilityRule(true)
	, NaniteSuitabilityIssueSeverity(EAssetIssueSeverity::Warning)
	, NaniteSuitabilityThreshold(5000)  // 5k triangles - should use Nanite
	, NaniteDisableThreshold(1000)      // 1k triangles - disable Nanite for performance
	, bAllowNaniteSuitabilityAutoFix(true)  // Allow automatic Nanite optimization
	, bEnableStaticMeshMaterialSlotRule(true)
	, MaterialSlotIssueSeverity(EAssetIssueSeverity::Warning)
	, MaterialSlotWarningThreshold(4)   // 4+ slots - warning
	, MaterialSlotErrorThreshold(6)     // 6+ slots - error
	, bAllowMaterialSlotAutoFix(true)   // Allow automatic slot optimization
	, bEnableStaticMeshVertexColorMissingRule(true)
	, VertexColorMissingIssueSeverity(EAssetIssueSeverity::Warning)
	, VertexColorRequiredThreshold(1000)  // 1k triangles - require vertex colors
	, bAllowVertexColorMissingAutoFix(false)  // Disabled - UE 5.5 API limitations
	, bEnableVertexColorUnusedChannelRule(true)
	, VertexColorUnusedChannelIssueSeverity(EAssetIssueSeverity::Warning)
	, bEnableVertexColorChannelValidation(false)
	, RequiredVertexColorChannels(TEXT("Mask,Detail"))
	, bEnableStaticMeshTransformPivotRule(true)
	, TransformPivotIssueSeverity(EAssetIssueSeverity::Warning)
	, TransformPivotWarningDistance(50.0f)  // 50 units - warning
	, TransformPivotErrorDistance(200.0f)   // 200 units - error
	// TransformScaleWarningRatio removed - moved to Scaling section
	// Transform pivot auto-fix removed - use external DCC tools
	, bEnableZeroScaleDetection(true)
	, ZeroScaleIssueSeverity(EAssetIssueSeverity::Warning)
	, ZeroScaleThreshold(0.01f)             // 0.01 units minimum scale
	, bEnableAssetTypeSpecificPivotRules(false)
	, bEnableNonUniformScaleDetection(true)
	, NonUniformScaleIssueSeverity(EAssetIssueSeverity::Warning)
	, NonUniformScaleWarningRatio(2.0f)     // 2:1 ratio threshold
	, bEnableStaticMeshLightmapResolutionRule(true)
	, LightmapResolutionIssueSeverity(EAssetIssueSeverity::Warning)
	, LightmapResolutionMin(4)              // 16x16 minimum
	, LightmapResolutionMax(16)             // 65536x65536 maximum (but clamped to 32 in UI)
	, bAllowLightmapResolutionAutoFix(true) // Allow automatic resolution setting
	, bEnableStaticMeshSocketNamingRule(true)
	, SocketNamingIssueSeverity(EAssetIssueSeverity::Warning)
	, SocketNamingPrefix(TEXT("Socket_"))   // Default prefix
	, SocketTransformWarningDistance(100.0f) // 100 units from bounds
	, bAllowSocketNamingAutoFix(true)       // Allow automatic socket fixes
{
	// Initialize default LOD reduction percentages
	// These represent the reduction from the previous LOD level
	DefaultLODReductionPercentages.Add(30.0f); // LOD1: 30% reduction from LOD0
	DefaultLODReductionPercentages.Add(50.0f); // LOD2: 50% reduction from LOD1
	DefaultLODReductionPercentages.Add(70.0f); // LOD3: 70% reduction from LOD2
	
	// Initialize other settings to their defaults here
	// Note: We don't create the profile here because we're in a UObject constructor
	// The profile will be created lazily in GetActiveProfile() when first needed
}

FName UPipelineGuardianSettings::GetCategoryName() const
{
	return TEXT("Plugins"); // Or FName(TEXT("Plugins")) - TEXT() macro should be fine here.
	// You can also try: return TEXT("PipelineGuardian"); to give it its own top-level category in the sidebar.
}

UPipelineGuardianProfile* UPipelineGuardianSettings::GetActiveProfile() const
{
	// Return cached profile if valid
	if (CachedActiveProfile)
	{
		return CachedActiveProfile;
	}

	// Try to load the active profile
	if (!ActiveProfilePath.IsNull())
	{
		UObject* LoadedObject = ActiveProfilePath.TryLoad();
		if (UPipelineGuardianProfile* LoadedProfile = Cast<UPipelineGuardianProfile>(LoadedObject))
		{
			CachedActiveProfile = LoadedProfile;
			return LoadedProfile;
		}
		else
		{
			UE_LOG(LogPipelineGuardian, Warning, TEXT("Failed to load active profile from path: %s"), *ActiveProfilePath.ToString());
		}
	}

	// Create a default profile if none exists
	if (!CachedActiveProfile)
	{
		// Create a transient profile that's not tied to the asset system
		CachedActiveProfile = NewObject<UPipelineGuardianProfile>(GetTransientPackage(), UPipelineGuardianProfile::StaticClass(), NAME_None, RF_Transient);
		if (CachedActiveProfile)
		{
			UE_LOG(LogPipelineGuardian, Log, TEXT("UPipelineGuardianSettings: Created default transient profile on-demand"));
			// Sync the quick settings to the newly created profile
			const_cast<UPipelineGuardianSettings*>(this)->SyncQuickSettingsToProfile();
		}
	}

	return CachedActiveProfile;
}

bool UPipelineGuardianSettings::SetActiveProfile(const FSoftObjectPath& ProfilePath)
{
	// Validate the path points to a valid profile
	UObject* LoadedObject = ProfilePath.TryLoad();
	if (UPipelineGuardianProfile* Profile = Cast<UPipelineGuardianProfile>(LoadedObject))
	{
		ActiveProfilePath = ProfilePath;
		CachedActiveProfile = Profile;
		
		// Add to available profiles if not already there
		AvailableProfiles.AddUnique(ProfilePath);
		
		// Save the settings
		SaveConfig();
		
		UE_LOG(LogPipelineGuardian, Log, TEXT("Set active profile to: %s"), *Profile->ProfileName);
		return true;
	}

	UE_LOG(LogPipelineGuardian, Error, TEXT("Failed to set active profile - invalid path: %s"), *ProfilePath.ToString());
	return false;
}

UPipelineGuardianProfile* UPipelineGuardianSettings::CreateNewProfile(const FString& ProfileName, const FString& Description)
{
	// Create a new profile asset
	UPipelineGuardianProfile* NewProfile = NewObject<UPipelineGuardianProfile>();
	if (!NewProfile)
	{
		UE_LOG(LogPipelineGuardian, Error, TEXT("Failed to create new profile object"));
		return nullptr;
	}

	NewProfile->ProfileName = ProfileName;
	NewProfile->Description = Description.IsEmpty() ? FString::Printf(TEXT("Profile: %s"), *ProfileName) : Description;
	NewProfile->Version = 1;

	// For now, we'll keep it in memory. In a full implementation, you'd want to save it as an asset
	// This would require asset creation utilities which are more complex
	CachedActiveProfile = NewProfile;
	
	UE_LOG(LogPipelineGuardian, Log, TEXT("Created new profile: %s"), *ProfileName);
	return NewProfile;
}

void UPipelineGuardianSettings::SyncQuickSettingsToProfile()
{
	UPipelineGuardianProfile* ActiveProfile = GetActiveProfile();
	if (!ActiveProfile)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("UPipelineGuardianSettings::SyncQuickSettingsToProfile: No active profile"));
		return;
	}

	// Sync Static Mesh Naming Rule
	FPipelineGuardianRuleConfig SMNamingRule(FName(TEXT("SM_Naming")), bEnableStaticMeshNamingRule);
	SMNamingRule.Parameters.Add(TEXT("NamingPattern"), StaticMeshNamingPattern);
	ActiveProfile->SetRuleConfig(SMNamingRule);

	// Sync Static Mesh LOD Rule
	FPipelineGuardianRuleConfig SMLODRule(FName(TEXT("SM_LODMissing")), bEnableStaticMeshLODRule);
	SMLODRule.Parameters.Add(TEXT("MinLODs_SM"), FString::FromInt(MinRequiredLODs));
	ActiveProfile->SetRuleConfig(SMLODRule);

	// Sync Static Mesh Lightmap UV Rule
	FPipelineGuardianRuleConfig SMLightmapUVRule(FName(TEXT("SM_LightmapUVMissing")), bEnableStaticMeshLightmapUVRule);
	FString SeverityString = TEXT("Warning");
	if (LightmapUVIssueSeverity == EAssetIssueSeverity::Error)
	{
		SeverityString = TEXT("Error");
	}
	else if (LightmapUVIssueSeverity == EAssetIssueSeverity::Info)
	{
		SeverityString = TEXT("Info");
	}
	SMLightmapUVRule.Parameters.Add(TEXT("Severity"), SeverityString);
	SMLightmapUVRule.Parameters.Add(TEXT("RequireValidUVs"), bRequireValidLightmapUVs ? TEXT("true") : TEXT("false"));
	SMLightmapUVRule.Parameters.Add(TEXT("AllowAutoGeneration"), bAllowLightmapUVAutoFix ? TEXT("true") : TEXT("false"));
	
	// Add channel strategy parameters
	FString StrategyString;
	switch (LightmapUVChannelStrategy)
	{
		case ELightmapUVChannelStrategy::NextAvailable:
			StrategyString = TEXT("NextAvailable");
			break;
		case ELightmapUVChannelStrategy::PreferredChannel:
			StrategyString = TEXT("PreferredChannel");
			break;
		case ELightmapUVChannelStrategy::ForceChannel1:
			StrategyString = TEXT("ForceChannel1");
			break;
		default:
			StrategyString = TEXT("NextAvailable");
			break;
	}
	SMLightmapUVRule.Parameters.Add(TEXT("ChannelStrategy"), StrategyString);
	SMLightmapUVRule.Parameters.Add(TEXT("PreferredChannel"), FString::FromInt(PreferredLightmapUVChannel));
	
	ActiveProfile->SetRuleConfig(SMLightmapUVRule);

	// UV Overlapping Rule Configuration
	FPipelineGuardianRuleConfig SMUVOverlappingRule(FName(TEXT("SM_UVOverlapping")), bEnableStaticMeshUVOverlappingRule);
	FString UVSeverityString = (UVOverlappingIssueSeverity == EAssetIssueSeverity::Error) ? TEXT("Error") : ((UVOverlappingIssueSeverity == EAssetIssueSeverity::Info) ? TEXT("Info") : TEXT("Warning"));
	SMUVOverlappingRule.Parameters.Add(TEXT("Severity"), UVSeverityString);
	SMUVOverlappingRule.Parameters.Add(TEXT("CheckUVChannel0"), bCheckUVChannel0 ? TEXT("true") : TEXT("false"));
	SMUVOverlappingRule.Parameters.Add(TEXT("CheckUVChannel1"), bCheckUVChannel1 ? TEXT("true") : TEXT("false"));
	SMUVOverlappingRule.Parameters.Add(TEXT("CheckUVChannel2"), bCheckUVChannel2 ? TEXT("true") : TEXT("false"));
	SMUVOverlappingRule.Parameters.Add(TEXT("CheckUVChannel3"), bCheckUVChannel3 ? TEXT("true") : TEXT("false"));
	SMUVOverlappingRule.Parameters.Add(TEXT("TextureUVTolerance"), FString::SanitizeFloat(TextureUVOverlapTolerance));
	SMUVOverlappingRule.Parameters.Add(TEXT("LightmapUVTolerance"), FString::SanitizeFloat(LightmapUVOverlapTolerance));
	SMUVOverlappingRule.Parameters.Add(TEXT("TextureWarningThreshold"), FString::SanitizeFloat(TextureUVOverlapWarningThreshold));
	SMUVOverlappingRule.Parameters.Add(TEXT("TextureErrorThreshold"), FString::SanitizeFloat(TextureUVOverlapErrorThreshold));
	SMUVOverlappingRule.Parameters.Add(TEXT("LightmapWarningThreshold"), FString::SanitizeFloat(LightmapUVOverlapWarningThreshold));
	SMUVOverlappingRule.Parameters.Add(TEXT("LightmapErrorThreshold"), FString::SanitizeFloat(LightmapUVOverlapErrorThreshold));
	// Note: AllowAutoFix parameter removed - UV fixes should be done in external tools
	
	ActiveProfile->SetRuleConfig(SMUVOverlappingRule);

	// Triangle Count Rule configuration
	FPipelineGuardianRuleConfig SMTriangleCountRule;
	SMTriangleCountRule.RuleID = TEXT("SM_TriangleCount");
	SMTriangleCountRule.bEnabled = bEnableStaticMeshTriangleCountRule;
	SMTriangleCountRule.Parameters.Add(TEXT("Severity"), FString::FromInt(static_cast<int32>(TriangleCountIssueSeverity)));
	SMTriangleCountRule.Parameters.Add(TEXT("BaseThreshold"), FString::FromInt(TriangleCountBaseThreshold));
	SMTriangleCountRule.Parameters.Add(TEXT("WarningPercentage"), FString::SanitizeFloat(TriangleCountWarningPercentage));
	SMTriangleCountRule.Parameters.Add(TEXT("ErrorPercentage"), FString::SanitizeFloat(TriangleCountErrorPercentage));
	// Note: Auto-fix parameters removed - use external 3D tools for mesh optimization
	
	ActiveProfile->SetRuleConfig(SMTriangleCountRule);

	// Degenerate Faces Rule configuration
	FPipelineGuardianRuleConfig SMDegenerateFacesRule;
	SMDegenerateFacesRule.RuleID = TEXT("SM_DegenerateFaces");
	SMDegenerateFacesRule.bEnabled = bEnableStaticMeshDegenerateFacesRule;
	SMDegenerateFacesRule.Parameters.Add(TEXT("Severity"), FString::FromInt(static_cast<int32>(DegenerateFacesIssueSeverity)));
	SMDegenerateFacesRule.Parameters.Add(TEXT("WarningThreshold"), FString::SanitizeFloat(DegenerateFacesWarningThreshold));
	SMDegenerateFacesRule.Parameters.Add(TEXT("ErrorThreshold"), FString::SanitizeFloat(DegenerateFacesErrorThreshold));
	SMDegenerateFacesRule.Parameters.Add(TEXT("AllowAutoFix"), bAllowDegenerateFacesAutoFix ? TEXT("true") : TEXT("false"));
	
	ActiveProfile->SetRuleConfig(SMDegenerateFacesRule);

	// Collision Missing Rule configuration
	FPipelineGuardianRuleConfig SMCollisionMissingRule;
	SMCollisionMissingRule.RuleID = TEXT("SM_CollisionMissing");
	SMCollisionMissingRule.bEnabled = bEnableStaticMeshCollisionMissingRule;
	SMCollisionMissingRule.Parameters.Add(TEXT("Severity"), FString::FromInt(static_cast<int32>(CollisionMissingIssueSeverity)));
	SMCollisionMissingRule.Parameters.Add(TEXT("AllowAutoFix"), bAllowCollisionMissingAutoFix ? TEXT("true") : TEXT("false"));
	
	ActiveProfile->SetRuleConfig(SMCollisionMissingRule);

	// Collision Complexity Rule configuration
	FPipelineGuardianRuleConfig SMCollisionComplexityRule;
	SMCollisionComplexityRule.RuleID = TEXT("SM_CollisionComplexity");
	SMCollisionComplexityRule.bEnabled = bEnableStaticMeshCollisionComplexityRule;
	SMCollisionComplexityRule.Parameters.Add(TEXT("Severity"), FString::FromInt(static_cast<int32>(CollisionComplexityIssueSeverity)));
	SMCollisionComplexityRule.Parameters.Add(TEXT("WarningThreshold"), FString::FromInt(CollisionComplexityWarningThreshold));
	SMCollisionComplexityRule.Parameters.Add(TEXT("ErrorThreshold"), FString::FromInt(CollisionComplexityErrorThreshold));
	SMCollisionComplexityRule.Parameters.Add(TEXT("TreatUseComplexAsSimpleAsError"), bTreatUseComplexAsSimpleAsError ? TEXT("true") : TEXT("false"));
	SMCollisionComplexityRule.Parameters.Add(TEXT("AllowAutoFix"), bAllowCollisionComplexityAutoFix ? TEXT("true") : TEXT("false"));
	
	ActiveProfile->SetRuleConfig(SMCollisionComplexityRule);

	// Nanite Suitability Rule configuration
	FPipelineGuardianRuleConfig SMNaniteSuitabilityRule;
	SMNaniteSuitabilityRule.RuleID = TEXT("SM_NaniteSuitability");
	SMNaniteSuitabilityRule.bEnabled = bEnableStaticMeshNaniteSuitabilityRule;
	SMNaniteSuitabilityRule.Parameters.Add(TEXT("Severity"), FString::FromInt(static_cast<int32>(NaniteSuitabilityIssueSeverity)));
	SMNaniteSuitabilityRule.Parameters.Add(TEXT("SuitabilityThreshold"), FString::FromInt(NaniteSuitabilityThreshold));
	SMNaniteSuitabilityRule.Parameters.Add(TEXT("DisableThreshold"), FString::FromInt(NaniteDisableThreshold));
	SMNaniteSuitabilityRule.Parameters.Add(TEXT("AllowAutoFix"), bAllowNaniteSuitabilityAutoFix ? TEXT("true") : TEXT("false"));
	ActiveProfile->SetRuleConfig(SMNaniteSuitabilityRule);

	// Material Slot Management Rule configuration
	FPipelineGuardianRuleConfig SMMaterialSlotRule;
	SMMaterialSlotRule.RuleID = TEXT("SM_MaterialSlot");
	SMMaterialSlotRule.bEnabled = bEnableStaticMeshMaterialSlotRule;
	SMMaterialSlotRule.Parameters.Add(TEXT("Severity"), FString::FromInt(static_cast<int32>(MaterialSlotIssueSeverity)));
	SMMaterialSlotRule.Parameters.Add(TEXT("WarningThreshold"), FString::FromInt(MaterialSlotWarningThreshold));
	SMMaterialSlotRule.Parameters.Add(TEXT("ErrorThreshold"), FString::FromInt(MaterialSlotErrorThreshold));
	SMMaterialSlotRule.Parameters.Add(TEXT("AllowAutoFix"), bAllowMaterialSlotAutoFix ? TEXT("true") : TEXT("false"));
	ActiveProfile->SetRuleConfig(SMMaterialSlotRule);

	// Vertex Color Missing Rule configuration
	FPipelineGuardianRuleConfig SMVertexColorMissingRule;
	SMVertexColorMissingRule.RuleID = TEXT("SM_VertexColorMissing");
	SMVertexColorMissingRule.bEnabled = bEnableStaticMeshVertexColorMissingRule;
	SMVertexColorMissingRule.Parameters.Add(TEXT("Severity"), FString::FromInt(static_cast<int32>(VertexColorMissingIssueSeverity)));
	SMVertexColorMissingRule.Parameters.Add(TEXT("RequiredThreshold"), FString::FromInt(VertexColorRequiredThreshold));
	SMVertexColorMissingRule.Parameters.Add(TEXT("AllowAutoFix"), bAllowVertexColorMissingAutoFix ? TEXT("true") : TEXT("false"));
	ActiveProfile->SetRuleConfig(SMVertexColorMissingRule);

	// Transform Pivot Rule configuration
	FPipelineGuardianRuleConfig SMTransformPivotRule;
	SMTransformPivotRule.RuleID = TEXT("SM_TransformPivot");
	SMTransformPivotRule.bEnabled = bEnableStaticMeshTransformPivotRule;
	SMTransformPivotRule.Parameters.Add(TEXT("Severity"), FString::FromInt(static_cast<int32>(TransformPivotIssueSeverity)));
	SMTransformPivotRule.Parameters.Add(TEXT("WarningDistance"), FString::Printf(TEXT("%.2f"), TransformPivotWarningDistance));
	SMTransformPivotRule.Parameters.Add(TEXT("ErrorDistance"), FString::Printf(TEXT("%.2f"), TransformPivotErrorDistance));
	// ScaleWarningRatio and AllowAutoFix removed - moved to Scaling section and disabled respectively
	ActiveProfile->SetRuleConfig(SMTransformPivotRule);

	// Scaling Rule configuration
	FPipelineGuardianRuleConfig SMScalingRule;
	SMScalingRule.RuleID = TEXT("SM_Scaling");
	SMScalingRule.bEnabled = bEnableNonUniformScaleDetection;
	SMScalingRule.Parameters.Add(TEXT("Severity"), FString::FromInt(static_cast<int32>(NonUniformScaleIssueSeverity)));
	SMScalingRule.Parameters.Add(TEXT("WarningRatio"), FString::Printf(TEXT("%.2f"), NonUniformScaleWarningRatio));
	SMScalingRule.Parameters.Add(TEXT("ZeroScaleEnabled"), bEnableZeroScaleDetection ? TEXT("true") : TEXT("false"));
	SMScalingRule.Parameters.Add(TEXT("ZeroScaleSeverity"), FString::FromInt(static_cast<int32>(ZeroScaleIssueSeverity)));
	SMScalingRule.Parameters.Add(TEXT("ZeroScaleThreshold"), FString::Printf(TEXT("%.3f"), ZeroScaleThreshold));
	ActiveProfile->SetRuleConfig(SMScalingRule);

	// Lightmap Resolution Rule configuration
	FPipelineGuardianRuleConfig SMLightmapResolutionRule;
	SMLightmapResolutionRule.RuleID = TEXT("SM_LightmapResolution");
	SMLightmapResolutionRule.bEnabled = bEnableStaticMeshLightmapResolutionRule;
	SMLightmapResolutionRule.Parameters.Add(TEXT("Severity"), FString::FromInt(static_cast<int32>(LightmapResolutionIssueSeverity)));
	SMLightmapResolutionRule.Parameters.Add(TEXT("MinResolution"), FString::FromInt(LightmapResolutionMin));
	SMLightmapResolutionRule.Parameters.Add(TEXT("MaxResolution"), FString::FromInt(LightmapResolutionMax));
	SMLightmapResolutionRule.Parameters.Add(TEXT("AllowAutoFix"), bAllowLightmapResolutionAutoFix ? TEXT("true") : TEXT("false"));
	ActiveProfile->SetRuleConfig(SMLightmapResolutionRule);

	// Socket Naming Rule configuration
	FPipelineGuardianRuleConfig SMSocketNamingRule;
	SMSocketNamingRule.RuleID = TEXT("SM_SocketNaming");
	SMSocketNamingRule.bEnabled = bEnableStaticMeshSocketNamingRule;
	SMSocketNamingRule.Parameters.Add(TEXT("Severity"), FString::FromInt(static_cast<int32>(SocketNamingIssueSeverity)));
	SMSocketNamingRule.Parameters.Add(TEXT("NamingPrefix"), SocketNamingPrefix);
	SMSocketNamingRule.Parameters.Add(TEXT("TransformWarningDistance"), FString::Printf(TEXT("%.2f"), SocketTransformWarningDistance));
	SMSocketNamingRule.Parameters.Add(TEXT("AllowAutoFix"), bAllowSocketNamingAutoFix ? TEXT("true") : TEXT("false"));
	ActiveProfile->SetRuleConfig(SMSocketNamingRule);

	UE_LOG(LogPipelineGuardian, Log, TEXT("UPipelineGuardianSettings: Synced quick settings to active profile"));
}

bool UPipelineGuardianSettings::ExportActiveProfileToFile(const FString& FilePath)
{
	UPipelineGuardianProfile* ActiveProfile = GetActiveProfile();
	if (!ActiveProfile)
	{
		UE_LOG(LogPipelineGuardian, Error, TEXT("No active profile to export"));
		return false;
	}

	FString JSONString = ActiveProfile->ExportToJSON();
	if (FFileHelper::SaveStringToFile(JSONString, *FilePath))
	{
		UE_LOG(LogPipelineGuardian, Log, TEXT("Successfully exported profile to: %s"), *FilePath);
		return true;
	}

	UE_LOG(LogPipelineGuardian, Error, TEXT("Failed to export profile to file: %s"), *FilePath);
	return false;
}

UPipelineGuardianProfile* UPipelineGuardianSettings::ImportProfileFromFile(const FString& FilePath, bool bSetAsActive)
{
	FString JSONString;
	if (!FFileHelper::LoadFileToString(JSONString, *FilePath))
	{
		UE_LOG(LogPipelineGuardian, Error, TEXT("Failed to load profile file: %s"), *FilePath);
		return nullptr;
	}

	UPipelineGuardianProfile* NewProfile = NewObject<UPipelineGuardianProfile>();
	if (!NewProfile || !NewProfile->ImportFromJSON(JSONString))
	{
		UE_LOG(LogPipelineGuardian, Error, TEXT("Failed to import profile from JSON"));
		return nullptr;
	}

	if (bSetAsActive)
	{
		CachedActiveProfile = NewProfile;
		UE_LOG(LogPipelineGuardian, Log, TEXT("Imported and set active profile: %s"), *NewProfile->ProfileName);
	}
	else
	{
		UE_LOG(LogPipelineGuardian, Log, TEXT("Imported profile: %s"), *NewProfile->ProfileName);
	}

	return NewProfile;
} 