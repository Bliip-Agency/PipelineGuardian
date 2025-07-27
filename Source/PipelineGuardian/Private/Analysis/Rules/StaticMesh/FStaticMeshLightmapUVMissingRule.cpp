// Copyright Epic Games, Inc. All Rights Reserved.

#include "FStaticMeshLightmapUVMissingRule.h"
#include "Analysis/FAssetAnalysisResult.h"
#include "Analysis/FPipelineGuardianProfile.h"
#include "FPipelineGuardianSettings.h"
#include "Engine/StaticMesh.h"
#include "PipelineGuardian.h"
#include "StaticMeshResources.h"
#include "Modules/ModuleManager.h"
#include "Misc/MessageDialog.h"

#define LOCTEXT_NAMESPACE "FStaticMeshLightmapUVMissingRule"

FStaticMeshLightmapUVMissingRule::FStaticMeshLightmapUVMissingRule()
{
}

bool FStaticMeshLightmapUVMissingRule::Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults)
{
	UStaticMesh* StaticMesh = Cast<UStaticMesh>(Asset);
	if (!StaticMesh)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("FStaticMeshLightmapUVMissingRule: Asset is not a UStaticMesh"));
		return false;
	}

	if (!Profile)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("FStaticMeshLightmapUVMissingRule: No profile provided"));
		return false;
	}

	// Check if this rule is enabled in the profile
	if (!Profile->IsRuleEnabled(GetRuleID()))
	{
		UE_LOG(LogPipelineGuardian, Verbose, TEXT("FStaticMeshLightmapUVMissingRule: Rule is disabled in profile"));
		return false;
	}

	// Get rule parameters from profile
	FString SeverityParam = Profile->GetRuleParameter(GetRuleID(), TEXT("Severity"), TEXT("Warning"));
	FString AllowAutoGenerationParam = Profile->GetRuleParameter(GetRuleID(), TEXT("AllowAutoGeneration"), TEXT("true"));
	FString ChannelStrategyParam = Profile->GetRuleParameter(GetRuleID(), TEXT("ChannelStrategy"), TEXT("NextAvailable"));
	FString PreferredChannelParam = Profile->GetRuleParameter(GetRuleID(), TEXT("PreferredChannel"), TEXT("1"));
	
	EAssetIssueSeverity ConfiguredSeverity = EAssetIssueSeverity::Warning;
	if (SeverityParam.Equals(TEXT("Error"), ESearchCase::IgnoreCase))
	{
		ConfiguredSeverity = EAssetIssueSeverity::Error;
	}
	else if (SeverityParam.Equals(TEXT("Info"), ESearchCase::IgnoreCase))
	{
		ConfiguredSeverity = EAssetIssueSeverity::Info;
	}
	
	bool bAllowAutoGeneration = AllowAutoGenerationParam.ToBool();
	int32 PreferredChannel = FCString::Atoi(*PreferredChannelParam);
	
	// Determine channel selection strategy
	ELightmapUVChannelStrategy ChannelStrategy = ELightmapUVChannelStrategy::NextAvailable;
	if (ChannelStrategyParam == TEXT("PreferredChannel"))
	{
		ChannelStrategy = ELightmapUVChannelStrategy::PreferredChannel;
	}
	else if (ChannelStrategyParam == TEXT("ForceChannel1"))
	{
		ChannelStrategy = ELightmapUVChannelStrategy::ForceChannel1;
	}

	// Check lightmap UV configuration
	bool bHasLightmapIssue = false;
	FText IssueDescription;
	
	// First check if bGenerateLightmapUVs is enabled
	if (StaticMesh->GetSourceModel(0).BuildSettings.bGenerateLightmapUVs)
	{
		UE_LOG(LogPipelineGuardian, VeryVerbose, TEXT("FStaticMeshLightmapUVMissingRule: %s has bGenerateLightmapUVs enabled"), 
			*StaticMesh->GetName());
		
		// If auto-generation is enabled, we're good
		return false; // No issues found
	}
	
	// bGenerateLightmapUVs is disabled, so we need to check for manual UV channel
	int32 LightmapCoordinateIndex = GetLightmapCoordinateIndex(StaticMesh);
	int32 UVChannelCount = GetUVChannelCount(StaticMesh);
	
	UE_LOG(LogPipelineGuardian, Warning, TEXT("FStaticMeshLightmapUVMissingRule DEBUG: %s - LightmapCoordIndex=%d, UVChannelCount=%d, bGenerateLightmapUVs=false"), 
		*StaticMesh->GetName(), LightmapCoordinateIndex, UVChannelCount);
	
	// Check for problematic configurations
	if (LightmapCoordinateIndex >= UVChannelCount)
	{
		// Lightmap coordinate index points to a non-existent UV channel
		bHasLightmapIssue = true;
		IssueDescription = FText::Format(
			LOCTEXT("LightmapUVChannelMissing", "Static Mesh '{0}' has bGenerateLightmapUVs disabled but lightmap coordinate index ({1}) points to non-existent UV channel. Available UV channels: {2}"),
			FText::FromString(StaticMesh->GetName()),
			FText::AsNumber(LightmapCoordinateIndex),
			FText::AsNumber(UVChannelCount)
		);
	}
	else if (LightmapCoordinateIndex == 0 && UVChannelCount > 0)
	{
		// Using UV channel 0 for lightmapping is generally problematic because it's the main texture UV
		bHasLightmapIssue = true;
		IssueDescription = FText::Format(
			LOCTEXT("LightmapUVUsingChannel0", "Static Mesh '{0}' has bGenerateLightmapUVs disabled and is using UV channel 0 for lightmapping. This can cause lighting artifacts. Consider enabling Generate Lightmap UVs or using a dedicated lightmap UV channel."),
			FText::FromString(StaticMesh->GetName())
		);
	}
	else if (LightmapCoordinateIndex > 0 && !IsUVChannelValid(StaticMesh, LightmapCoordinateIndex))
	{
		// UV channel exists but has invalid UVs
		bHasLightmapIssue = true;
		IssueDescription = FText::Format(
			LOCTEXT("LightmapUVChannelInvalid", "Static Mesh '{0}' has bGenerateLightmapUVs disabled but UV channel {1} (used for lightmapping) contains invalid or all-zero UVs"),
			FText::FromString(StaticMesh->GetName()),
			FText::AsNumber(LightmapCoordinateIndex)
		);
	}
	
	if (bHasLightmapIssue)
	{
		FAssetAnalysisResult Result;
		Result.Asset = FAssetData(StaticMesh);
		Result.Severity = ConfiguredSeverity;
		Result.RuleID = GetRuleID();
		Result.Description = IssueDescription;
		Result.FilePath = FText::FromString(StaticMesh->GetPackage()->GetName());
		
		// Create fix actions if allowed
		if (bAllowAutoGeneration)
		{
			Result.FixAction.BindLambda([StaticMesh, ChannelStrategy, PreferredChannel]()
			{
				// Determine the best UV channel to use
				int32 DestinationChannel = DetermineOptimalLightmapUVChannel(StaticMesh, ChannelStrategy, PreferredChannel);
				
				// Primary fix: Enable bGenerateLightmapUVs with smart channel selection
				EnableGenerateLightmapUVs(StaticMesh, DestinationChannel);
			});
		}
		
		OutResults.Add(Result);
		
		UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshLightmapUVMissingRule: Lightmap UV issue found for %s"), 
			*StaticMesh->GetName());
		return true; // Issue found
	}
	else
	{
		UE_LOG(LogPipelineGuardian, VeryVerbose, TEXT("FStaticMeshLightmapUVMissingRule: %s has proper lightmap UV configuration"), 
			*StaticMesh->GetName());
		return false; // No issues found
	}
}

FName FStaticMeshLightmapUVMissingRule::GetRuleID() const
{
	return FName(TEXT("SM_LightmapUVMissing"));
}

FText FStaticMeshLightmapUVMissingRule::GetRuleDescription() const
{
	return LOCTEXT("StaticMeshLightmapUVMissingRuleDescription", "Validates that Static Mesh assets have proper lightmap UV configuration - either bGenerateLightmapUVs enabled or valid UV channel for lightmapping.");
}

bool FStaticMeshLightmapUVMissingRule::HasValidLightmapUVChannel(UStaticMesh* StaticMesh) const
{
	if (!StaticMesh)
	{
		return false;
	}
	
	int32 LightmapCoordinateIndex = GetLightmapCoordinateIndex(StaticMesh);
	int32 UVChannelCount = GetUVChannelCount(StaticMesh);
	
	// Check if the lightmap coordinate index is within valid range
	return LightmapCoordinateIndex < UVChannelCount;
}

int32 FStaticMeshLightmapUVMissingRule::GetUVChannelCount(UStaticMesh* StaticMesh) const
{
	if (!StaticMesh || !StaticMesh->GetRenderData() || StaticMesh->GetRenderData()->LODResources.Num() == 0)
	{
		return 0;
	}
	
	const FStaticMeshLODResources& LODResource = StaticMesh->GetRenderData()->LODResources[0];
	return LODResource.GetNumTexCoords();
}

bool FStaticMeshLightmapUVMissingRule::IsUVChannelValid(UStaticMesh* StaticMesh, int32 UVChannelIndex) const
{
	if (!StaticMesh || !StaticMesh->GetRenderData() || StaticMesh->GetRenderData()->LODResources.Num() == 0)
	{
		return false;
	}
	
	const FStaticMeshLODResources& LODResource = StaticMesh->GetRenderData()->LODResources[0];
	
	// Check if UV channel index is valid
	if (UVChannelIndex >= LODResource.GetNumTexCoords())
	{
		return false;
	}
	
	// Get vertex count
	int32 NumVertices = LODResource.GetNumVertices();
	if (NumVertices == 0)
	{
		return false;
	}
	
	// Basic validation - check if UVs exist and are not all zero
	const FStaticMeshVertexBuffer& VertexBuffer = LODResource.VertexBuffers.StaticMeshVertexBuffer;
	
	bool bHasNonZeroUVs = false;
	for (int32 VertexIndex = 0; VertexIndex < FMath::Min(NumVertices, 100); ++VertexIndex) // Sample first 100 vertices
	{
		FVector2f UV = VertexBuffer.GetVertexUV(VertexIndex, UVChannelIndex);
		if (!UV.IsZero())
		{
			bHasNonZeroUVs = true;
			break;
		}
	}
	
	return bHasNonZeroUVs;
}

void FStaticMeshLightmapUVMissingRule::GenerateLightmapUVs(UStaticMesh* StaticMesh)
{
	if (!StaticMesh)
	{
		UE_LOG(LogPipelineGuardian, Error, TEXT("FStaticMeshLightmapUVMissingRule::GenerateLightmapUVs: StaticMesh is null"));
		return;
	}
	
	// For now, we'll just enable bGenerateLightmapUVs as the primary fix
	EnableGenerateLightmapUVs(StaticMesh, 1); // Use default UV channel 1
}

void FStaticMeshLightmapUVMissingRule::EnableGenerateLightmapUVs(UStaticMesh* StaticMesh, int32 DestinationUVChannel)
{
	if (!StaticMesh)
	{
		UE_LOG(LogPipelineGuardian, Error, TEXT("FStaticMeshLightmapUVMissingRule::EnableGenerateLightmapUVs: StaticMesh is null"));
		return;
	}
	
	// Clamp destination channel to valid range (1-7)
	DestinationUVChannel = FMath::Clamp(DestinationUVChannel, 1, 7);
	
	// Modify the static mesh
	StaticMesh->Modify();
	
	// Enable bGenerateLightmapUVs in build settings
	FMeshBuildSettings& BuildSettings = StaticMesh->GetSourceModel(0).BuildSettings;
	BuildSettings.bGenerateLightmapUVs = true;
	
	// Set reasonable defaults for lightmap UV generation
	BuildSettings.MinLightmapResolution = 64;
	BuildSettings.SrcLightmapIndex = 0;  // Use UV channel 0 as source
	BuildSettings.DstLightmapIndex = DestinationUVChannel;  // Generate lightmap UVs in specified channel
	
	// Set lightmap coordinate index to the destination channel
	StaticMesh->SetLightMapCoordinateIndex(BuildSettings.DstLightmapIndex);
	
	// Build the mesh to apply changes
	StaticMesh->Build();
	
	UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshLightmapUVMissingRule: Enabled bGenerateLightmapUVs for '%s' with destination UV channel %d"), 
		*StaticMesh->GetName(), DestinationUVChannel);
	
	// Show success message
	FText Message = FText::Format(
		LOCTEXT("LightmapUVGenerationEnabled", "Successfully enabled automatic lightmap UV generation for '{0}'.\n\nLightmap UVs will be generated in UV channel {1} during builds."),
		FText::FromString(StaticMesh->GetName()),
		FText::AsNumber(DestinationUVChannel)
	);
	FMessageDialog::Open(EAppMsgType::Ok, Message, LOCTEXT("LightmapUVGenerationSuccess", "Lightmap UV Generation Enabled"));
}

int32 FStaticMeshLightmapUVMissingRule::DetermineOptimalLightmapUVChannel(UStaticMesh* StaticMesh, ELightmapUVChannelStrategy Strategy, int32 PreferredChannel)
{
	if (!StaticMesh)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("DetermineOptimalLightmapUVChannel: StaticMesh is null, defaulting to channel 1"));
		return 1;
	}

	switch (Strategy)
	{
		case ELightmapUVChannelStrategy::NextAvailable:
		{
			// Find the next available UV channel
			return FindNextAvailableUVChannel(StaticMesh, 1);
		}
		
		case ELightmapUVChannelStrategy::PreferredChannel:
		{
			// Check if preferred channel is available, otherwise find next available
			if (!HasValidUVData(StaticMesh, PreferredChannel))
			{
				UE_LOG(LogPipelineGuardian, Log, TEXT("DetermineOptimalLightmapUVChannel: Using preferred channel %d"), PreferredChannel);
				return PreferredChannel;
			}
			else
			{
				int32 NextAvailable = FindNextAvailableUVChannel(StaticMesh, PreferredChannel + 1);
				UE_LOG(LogPipelineGuardian, Log, TEXT("DetermineOptimalLightmapUVChannel: Preferred channel %d occupied, using next available: %d"), PreferredChannel, NextAvailable);
				return NextAvailable;
			}
		}
		
		case ELightmapUVChannelStrategy::ForceChannel1:
		default:
		{
			// Legacy behavior - always use channel 1
			UE_LOG(LogPipelineGuardian, Log, TEXT("DetermineOptimalLightmapUVChannel: Force using channel 1"));
			return 1;
		}
	}
}

int32 FStaticMeshLightmapUVMissingRule::FindNextAvailableUVChannel(UStaticMesh* StaticMesh, int32 StartFromChannel)
{
	if (!StaticMesh)
	{
		return 1;
	}

	// Search from StartFromChannel up to 7 (max UV channels supported by UE)
	for (int32 Channel = StartFromChannel; Channel <= 7; ++Channel)
	{
		if (!HasValidUVData(StaticMesh, Channel))
		{
			UE_LOG(LogPipelineGuardian, Log, TEXT("FindNextAvailableUVChannel: Found available channel %d"), Channel);
			return Channel;
		}
	}

	// If no empty channel found, warn and return the start channel
	UE_LOG(LogPipelineGuardian, Warning, TEXT("FindNextAvailableUVChannel: No empty UV channels found, defaulting to channel %d"), StartFromChannel);
	return StartFromChannel;
}

bool FStaticMeshLightmapUVMissingRule::HasValidUVData(UStaticMesh* StaticMesh, int32 UVChannel)
{
	if (!StaticMesh || UVChannel < 0)
	{
		return false;
	}

	// Check if the mesh has LOD 0 data
	if (StaticMesh->GetNumLODs() == 0)
	{
		return false;
	}

	const FStaticMeshLODResources& LODResource = StaticMesh->GetRenderData()->LODResources[0];
	
	// Check if this UV channel exists
	if (UVChannel >= LODResource.GetNumTexCoords())
	{
		return false;
	}

	// Check if the UV channel has non-zero coordinates (indicating actual UV data)
	const FRawStaticIndexBuffer& IndexBuffer = LODResource.IndexBuffer;
	const FStaticMeshVertexBuffer& VertexBuffer = LODResource.VertexBuffers.StaticMeshVertexBuffer;
	
	// Sample a few vertices to check if they have meaningful UV data
	int32 NumVerticesToCheck = FMath::Min(10, (int32)VertexBuffer.GetNumVertices());
	int32 NonZeroUVCount = 0;
	
	for (int32 VertexIndex = 0; VertexIndex < NumVerticesToCheck; ++VertexIndex)
	{
		FVector2f UV = VertexBuffer.GetVertexUV(VertexIndex, UVChannel);
		
		// Consider UV valid if it's not exactly (0,0) or if it has reasonable values
		if (!UV.IsNearlyZero(KINDA_SMALL_NUMBER))
		{
			NonZeroUVCount++;
		}
	}

	// If more than half the sampled vertices have non-zero UVs, consider channel occupied
	bool bHasValidData = (NonZeroUVCount > NumVerticesToCheck / 2);
	
	UE_LOG(LogPipelineGuardian, VeryVerbose, TEXT("HasValidUVData: Channel %d has %d/%d non-zero UVs, considered %s"), 
		UVChannel, NonZeroUVCount, NumVerticesToCheck, bHasValidData ? TEXT("occupied") : TEXT("available"));
		
	return bHasValidData;
}

bool FStaticMeshLightmapUVMissingRule::CanGenerateLightmapUVs(UStaticMesh* StaticMesh) const
{
	if (!StaticMesh)
	{
		return false;
	}
	
	// Check if mesh has valid geometry
	if (!StaticMesh->GetRenderData() || StaticMesh->GetRenderData()->LODResources.Num() == 0)
	{
		return false;
	}
	
	// Check if base LOD has vertices
	const FStaticMeshLODResources& BaseLOD = StaticMesh->GetRenderData()->LODResources[0];
	if (BaseLOD.GetNumVertices() == 0)
	{
		return false;
	}
	
	// We can always enable bGenerateLightmapUVs as it's a build setting
	return true;
}

int32 FStaticMeshLightmapUVMissingRule::GetLightmapCoordinateIndex(UStaticMesh* StaticMesh) const
{
	if (!StaticMesh)
	{
		return 1; // Default to UV channel 1
	}
	
	return StaticMesh->GetLightMapCoordinateIndex();
}

#undef LOCTEXT_NAMESPACE 