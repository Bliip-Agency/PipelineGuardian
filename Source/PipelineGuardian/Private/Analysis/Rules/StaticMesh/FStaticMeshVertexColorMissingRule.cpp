#include "FStaticMeshVertexColorMissingRule.h"
#include "Analysis/FAssetAnalysisResult.h"
#include "FPipelineGuardianSettings.h"
#include "PipelineGuardian.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSourceData.h"
#include "StaticMeshResources.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "Misc/MessageDialog.h"

FStaticMeshVertexColorMissingRule::FStaticMeshVertexColorMissingRule()
{
}

bool FStaticMeshVertexColorMissingRule::Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults)
{
	UStaticMesh* StaticMesh = Cast<UStaticMesh>(Asset);
	if (!StaticMesh)
	{
		return false;
	}

	// Get settings
	const UPipelineGuardianSettings* Settings = GetDefault<UPipelineGuardianSettings>();
	if (!Settings)
	{
		return false;
	}

	bool HasIssues = false;

	// Check for missing vertex colors
	if (Settings->bEnableStaticMeshVertexColorMissingRule)
	{
		// Get triangle count
		int32 TriangleCount = 0;
		if (StaticMesh->GetRenderData() && StaticMesh->GetRenderData()->LODResources.Num() > 0)
		{
			TriangleCount = StaticMesh->GetRenderData()->LODResources[0].GetNumTriangles();
		}

		// Check if vertex colors are missing
		bool HasMissingVertexColors = this->HasMissingVertexColors(StaticMesh, Settings->VertexColorRequiredThreshold);

		if (HasMissingVertexColors)
		{
			FAssetAnalysisResult Result;
			Result.Asset = FAssetData(StaticMesh);
			Result.RuleID = GetRuleID();
			Result.Severity = Settings->VertexColorMissingIssueSeverity;
			Result.Description = FText::FromString(GenerateVertexColorMissingDescription(StaticMesh, TriangleCount, Settings->VertexColorRequiredThreshold));

			// Add fix action if auto-fix is enabled
			if (Settings->bAllowVertexColorMissingAutoFix && CanSafelyGenerateVertexColors(StaticMesh))
			{
				Result.FixAction = FSimpleDelegate::CreateLambda([this, StaticMesh]()
				{
					if (GenerateVertexColors(StaticMesh))
					{
						UE_LOG(LogPipelineGuardian, Log, TEXT("Successfully generated vertex colors for %s"), *StaticMesh->GetName());
					}
					else
					{
						UE_LOG(LogPipelineGuardian, Warning, TEXT("Failed to generate vertex colors for %s"), *StaticMesh->GetName());
					}
				});
			}

			OutResults.Add(Result);
			HasIssues = true;
		}
	}

	// Check for unused vertex color channels
	if (Settings->bEnableVertexColorUnusedChannelRule)
	{
		TArray<FString> UnusedChannels;
		bool HasUnusedChannels = this->HasUnusedVertexColorChannels(StaticMesh, UnusedChannels);

		if (HasUnusedChannels)
		{
			FAssetAnalysisResult Result;
			Result.Asset = FAssetData(StaticMesh);
			Result.RuleID = FName(TEXT("SM_VertexColorUnusedChannels"));
			Result.Severity = Settings->VertexColorUnusedChannelIssueSeverity;
			Result.Description = FText::FromString(GenerateUnusedChannelDescription(StaticMesh, UnusedChannels));

			// No auto-fix for unused channel detection - this is informational only
			// Result.FixAction = FSimpleDelegate::CreateLambda([this, StaticMesh, UnusedChannels]()
			// {
			// 	if (OptimizeVertexColors(StaticMesh, UnusedChannels))
			// 	{
			// 		UE_LOG(LogPipelineGuardian, Log, TEXT("Successfully optimized vertex colors for %s"), *StaticMesh->GetName());
			// 	}
			// 	else
			// 	{
			// 		UE_LOG(LogPipelineGuardian, Warning, TEXT("Failed to optimize vertex colors for %s"), *StaticMesh->GetName());
			// 	}
			// });

			OutResults.Add(Result);
			HasIssues = true;
		}
	}

	// Check for missing required channels
	if (Settings->bEnableVertexColorChannelValidation && !Settings->RequiredVertexColorChannels.IsEmpty())
	{
		TArray<FString> MissingChannels;
		bool HasMissingChannels = this->HasMissingRequiredChannels(StaticMesh, Settings->RequiredVertexColorChannels, MissingChannels);

		if (HasMissingChannels)
		{
			FAssetAnalysisResult Result;
			Result.Asset = FAssetData(StaticMesh);
			Result.RuleID = FName(TEXT("SM_VertexColorMissingChannels"));
			Result.Severity = Settings->VertexColorMissingIssueSeverity;
			Result.Description = FText::FromString(GenerateMissingChannelDescription(StaticMesh, MissingChannels));

			OutResults.Add(Result);
			HasIssues = true;
		}
	}

	return HasIssues;
}

FName FStaticMeshVertexColorMissingRule::GetRuleID() const
{
	return TEXT("SM_VertexColorMissing");
}

FText FStaticMeshVertexColorMissingRule::GetRuleDescription() const
{
	return FText::FromString(TEXT("Checks if static meshes are missing required vertex color channels based on polygon count."));
}

bool FStaticMeshVertexColorMissingRule::HasMissingVertexColors(const UStaticMesh* StaticMesh, int32 RequiredThreshold) const
{
	if (!StaticMesh || !StaticMesh->GetRenderData() || StaticMesh->GetRenderData()->LODResources.Num() == 0)
	{
		return false;
	}

	// Get triangle count
	int32 TriangleCount = StaticMesh->GetRenderData()->LODResources[0].GetNumTriangles();
	
	// Only check for vertex colors if mesh meets the threshold
	if (TriangleCount < RequiredThreshold)
	{
		return false;
	}

	// Check if vertex colors exist in the mesh description
	if (StaticMesh->GetMeshDescription(0))
	{
		const FMeshDescription* MeshDesc = StaticMesh->GetMeshDescription(0);
		const FStaticMeshConstAttributes Attributes(*MeshDesc);
		
		// Check if vertex color attributes exist
		if (Attributes.GetVertexInstanceColors().IsValid())
		{
			return false; // Vertex colors exist
		}
	}

	return true; // Vertex colors are missing
}

FString FStaticMeshVertexColorMissingRule::GenerateVertexColorMissingDescription(const UStaticMesh* StaticMesh, int32 TriangleCount, int32 RequiredThreshold) const
{
	return FString::Printf(TEXT("Static mesh %s (%d triangles) is missing vertex colors. Required for meshes with %d+ triangles. Vertex colors improve visual quality and support advanced shading."), 
		*StaticMesh->GetName(), TriangleCount, RequiredThreshold);
}

bool FStaticMeshVertexColorMissingRule::GenerateVertexColors(UStaticMesh* StaticMesh) const
{
	if (!StaticMesh)
	{
		return false;
	}

	UE_LOG(LogPipelineGuardian, Log, TEXT("Generating vertex colors for %s"), *StaticMesh->GetName());

	// Get the mesh description
	FMeshDescription* MeshDesc = StaticMesh->GetMeshDescription(0);
	if (!MeshDesc)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("Cannot generate vertex colors for %s: No mesh description"), *StaticMesh->GetName());
		return false;
	}

	// Add vertex color attributes if they don't exist
	FStaticMeshAttributes Attributes(*MeshDesc);
	if (!Attributes.GetVertexInstanceColors().IsValid())
	{
		// Note: RegisterVertexInstanceColor() is not available in UE 5.5
		// We'll need to handle this differently or disable auto-fix
		UE_LOG(LogPipelineGuardian, Warning, TEXT("Vertex color registration not available in UE 5.5 - auto-fix disabled"));
		return false;
	}

	// Get vertex color attribute
	TVertexInstanceAttributesRef<FVector4f> VertexColors = Attributes.GetVertexInstanceColors();
	
	// Generate simple vertex colors based on vertex positions
	for (FVertexInstanceID VertexInstanceID : MeshDesc->VertexInstances().GetElementIDs())
	{
		FVertexID VertexID = MeshDesc->GetVertexInstanceVertex(VertexInstanceID);
		FVector3f Position = MeshDesc->GetVertexPosition(VertexID);
		
		// Generate a simple color based on position (can be customized)
		FVector4f Color(
			FMath::Abs(Position.X) * 0.1f + 0.5f,  // Red component
			FMath::Abs(Position.Y) * 0.1f + 0.5f,  // Green component
			FMath::Abs(Position.Z) * 0.1f + 0.5f,  // Blue component
			1.0f  // Alpha component
		);
		
		// Clamp to valid range
		Color.X = FMath::Clamp(Color.X, 0.0f, 1.0f);
		Color.Y = FMath::Clamp(Color.Y, 0.0f, 1.0f);
		Color.Z = FMath::Clamp(Color.Z, 0.0f, 1.0f);
		Color.W = FMath::Clamp(Color.W, 0.0f, 1.0f);
		VertexColors[VertexInstanceID] = Color;
	}

	// Mark for rebuild
	StaticMesh->Build(false);
	StaticMesh->MarkPackageDirty();
	StaticMesh->PostEditChange();

	UE_LOG(LogPipelineGuardian, Log, TEXT("Successfully generated vertex colors for %s"), *StaticMesh->GetName());
	return true;
}

bool FStaticMeshVertexColorMissingRule::CanSafelyGenerateVertexColors(const UStaticMesh* StaticMesh) const
{
	if (!StaticMesh)
	{
		return false;
	}

	// Check if mesh has valid geometry
	if (!StaticMesh->GetRenderData() || StaticMesh->GetRenderData()->LODResources.Num() == 0)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("Cannot generate vertex colors for %s: No valid geometry"), *StaticMesh->GetName());
		return false;
	}

	// Check if mesh is not too complex for vertex color generation
	const FStaticMeshLODResources* LODResource = &StaticMesh->GetRenderData()->LODResources[0];
	int32 TriangleCount = LODResource->GetNumTriangles();
	
	// Don't auto-generate for extremely complex meshes (more than 100k triangles)
	if (TriangleCount > 100000)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("Cannot auto-generate vertex colors for %s: Too complex (%d triangles)"), 
			*StaticMesh->GetName(), TriangleCount);
		return false;
	}

	// Check if mesh description exists
	if (!StaticMesh->GetMeshDescription(0))
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("Cannot generate vertex colors for %s: No mesh description"), *StaticMesh->GetName());
		return false;
	}

	return true;
} 

bool FStaticMeshVertexColorMissingRule::HasUnusedVertexColorChannels(const UStaticMesh* StaticMesh, TArray<FString>& OutUnusedChannels) const
{
	if (!StaticMesh || !StaticMesh->GetRenderData() || StaticMesh->GetRenderData()->LODResources.Num() == 0)
	{
		return false;
	}

	// Get mesh description to analyze vertex color usage
	if (StaticMesh->GetMeshDescription(0))
	{
		const FMeshDescription* MeshDesc = StaticMesh->GetMeshDescription(0);
		const FStaticMeshConstAttributes Attributes(*MeshDesc);
		
		if (Attributes.GetVertexInstanceColors().IsValid())
		{
			TVertexInstanceAttributesConstRef<FVector4f> VertexColors = Attributes.GetVertexInstanceColors();
			
			// Analyze vertex color usage patterns
			bool HasNonZeroColors = false;
			bool HasVaryingColors = false;
			FVector4f FirstColor = FVector4f::Zero();
			bool FirstColorSet = false;
			
			for (FVertexInstanceID VertexInstanceID : MeshDesc->VertexInstances().GetElementIDs())
			{
				FVector4f Color = VertexColors[VertexInstanceID];
				
				// Check if any color component is non-zero
				if (Color.X > 0.0f || Color.Y > 0.0f || Color.Z > 0.0f || Color.W > 0.0f)
				{
					HasNonZeroColors = true;
				}
				
				// Check for color variation
				if (!FirstColorSet)
				{
					FirstColor = Color;
					FirstColorSet = true;
				}
				else if ((FirstColor - Color).SizeSquared() > 0.0001f)
				{
					HasVaryingColors = true;
				}
			}
			
			// Determine if channels are unused
			if (!HasNonZeroColors)
			{
				OutUnusedChannels.Add(TEXT("All"));
				return true;
			}
			else if (!HasVaryingColors)
			{
				OutUnusedChannels.Add(TEXT("Variation"));
				return true;
			}
		}
	}

	return false;
}

bool FStaticMeshVertexColorMissingRule::HasMissingRequiredChannels(const UStaticMesh* StaticMesh, const FString& RequiredChannels, TArray<FString>& OutMissingChannels) const
{
	if (!StaticMesh || RequiredChannels.IsEmpty())
	{
		return false;
	}

	// Parse required channels from comma-separated string
	TArray<FString> RequiredChannelList;
	RequiredChannels.ParseIntoArray(RequiredChannelList, TEXT(","), true);
	
	// For now, we'll check if vertex colors exist at all
	// In a more advanced implementation, you could check for specific channel names
	if (StaticMesh->GetMeshDescription(0))
	{
		const FMeshDescription* MeshDesc = StaticMesh->GetMeshDescription(0);
		const FStaticMeshConstAttributes Attributes(*MeshDesc);
		
		if (!Attributes.GetVertexInstanceColors().IsValid())
		{
			// If no vertex colors exist, all required channels are missing
			OutMissingChannels = RequiredChannelList;
			return true;
		}
	}
	else
	{
		// No mesh description means no vertex colors
		OutMissingChannels = RequiredChannelList;
		return true;
	}

	return false;
}

FString FStaticMeshVertexColorMissingRule::GenerateUnusedChannelDescription(const UStaticMesh* StaticMesh, const TArray<FString>& UnusedChannels) const
{
	FString ChannelList = TEXT("");
	for (int32 i = 0; i < UnusedChannels.Num(); ++i)
	{
		if (i > 0) ChannelList += TEXT(", ");
		ChannelList += UnusedChannels[i];
	}
	
	return FString::Printf(TEXT("Static mesh %s has unused vertex color channels: %s. This bloats data and should be optimized."), 
		*StaticMesh->GetName(), *ChannelList);
}

FString FStaticMeshVertexColorMissingRule::GenerateMissingChannelDescription(const UStaticMesh* StaticMesh, const TArray<FString>& MissingChannels) const
{
	FString ChannelList = TEXT("");
	for (int32 i = 0; i < MissingChannels.Num(); ++i)
	{
		if (i > 0) ChannelList += TEXT(", ");
		ChannelList += MissingChannels[i];
	}
	
	return FString::Printf(TEXT("Static mesh %s is missing required vertex color channels: %s. These channels are needed for proper shading."), 
		*StaticMesh->GetName(), *ChannelList);
}

bool FStaticMeshVertexColorMissingRule::OptimizeVertexColors(UStaticMesh* StaticMesh, const TArray<FString>& ChannelsToRemove) const
{
	if (!StaticMesh)
	{
		return false;
	}

	UE_LOG(LogPipelineGuardian, Log, TEXT("Optimizing vertex colors for %s"), *StaticMesh->GetName());

	// For now, we'll just log the optimization
	// In a full implementation, you would remove unused channels or optimize the data
	UE_LOG(LogPipelineGuardian, Warning, TEXT("Vertex color optimization not fully implemented in UE 5.5 - manual optimization required"));

	// Mark for rebuild
	StaticMesh->MarkPackageDirty();
	StaticMesh->PostEditChange();

	return true;
} 