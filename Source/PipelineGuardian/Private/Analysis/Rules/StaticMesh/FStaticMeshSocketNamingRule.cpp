#include "FStaticMeshSocketNamingRule.h"
#include "Analysis/FAssetAnalysisResult.h"
#include "FPipelineGuardianSettings.h"
#include "PipelineGuardian.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSourceData.h"
#include "StaticMeshResources.h"
#include "Engine/StaticMeshSocket.h"
#include "Misc/MessageDialog.h"

FStaticMeshSocketNamingRule::FStaticMeshSocketNamingRule()
{
}

bool FStaticMeshSocketNamingRule::Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults)
{
	UStaticMesh* StaticMesh = Cast<UStaticMesh>(Asset);
	if (!StaticMesh)
	{
		return false;
	}

	// Get settings
	const UPipelineGuardianSettings* Settings = GetDefault<UPipelineGuardianSettings>();
	if (!Settings || !Settings->bEnableStaticMeshSocketNamingRule)
	{
		return false;
	}

	bool HasIssue = false;
	EAssetIssueSeverity Severity = Settings->SocketNamingIssueSeverity;
	TArray<FString> InvalidNamingSockets;
	TArray<FString> InvalidTransformSockets;

	// Check socket naming conventions
	bool HasInvalidNaming = HasInvalidSocketNaming(StaticMesh, Settings->SocketNamingPrefix, InvalidNamingSockets);
	
	// Check socket transforms
	bool HasInvalidTransforms = HasInvalidSocketTransforms(StaticMesh, Settings->SocketTransformWarningDistance, InvalidTransformSockets);

	// Determine if there are issues
	if (HasInvalidNaming || HasInvalidTransforms)
	{
		HasIssue = true;
	}

	if (HasIssue)
	{
		FAssetAnalysisResult Result;
		Result.Asset = FAssetData(StaticMesh);
		Result.RuleID = GetRuleID();
		Result.Severity = Severity;
		Result.Description = FText::FromString(GenerateSocketNamingDescription(StaticMesh, InvalidNamingSockets, InvalidTransformSockets, Settings->SocketNamingPrefix));

		// Add fix action if auto-fix is enabled
		if (Settings->bAllowSocketNamingAutoFix && CanSafelyFixSocketIssues(StaticMesh))
		{
			Result.FixAction = FSimpleDelegate::CreateLambda([this, StaticMesh, Settings]()
			{
				if (FixSocketIssues(StaticMesh, Settings->SocketNamingPrefix))
				{
					UE_LOG(LogPipelineGuardian, Log, TEXT("Successfully fixed socket issues for %s"), *StaticMesh->GetName());
				}
				else
				{
					UE_LOG(LogPipelineGuardian, Warning, TEXT("Failed to fix socket issues for %s"), *StaticMesh->GetName());
				}
			});
		}

		OutResults.Add(Result);
		return true;
	}

	return false;
}

FName FStaticMeshSocketNamingRule::GetRuleID() const
{
	return TEXT("SM_SocketNaming");
}

FText FStaticMeshSocketNamingRule::GetRuleDescription() const
{
	return FText::FromString(TEXT("Checks for proper socket naming conventions and reasonable transform positions."));
}

bool FStaticMeshSocketNamingRule::HasInvalidSocketNaming(const UStaticMesh* StaticMesh, const FString& RequiredPrefix, TArray<FString>& OutInvalidSocketNames) const
{
	if (!StaticMesh)
	{
		return false;
	}

	OutInvalidSocketNames.Empty();

	// If no prefix is required, all names are valid
	if (RequiredPrefix.IsEmpty())
	{
		return false;
	}

	// Check all sockets for proper naming
	for (const UStaticMeshSocket* Socket : StaticMesh->Sockets)
	{
		if (Socket && !Socket->SocketName.ToString().StartsWith(RequiredPrefix))
		{
			OutInvalidSocketNames.Add(Socket->SocketName.ToString());
		}
	}

	return OutInvalidSocketNames.Num() > 0;
}

bool FStaticMeshSocketNamingRule::HasInvalidSocketTransforms(const UStaticMesh* StaticMesh, float WarningDistance, TArray<FString>& OutInvalidSocketNames) const
{
	if (!StaticMesh)
	{
		return false;
	}

	OutInvalidSocketNames.Empty();

	// Get mesh bounds for distance checking
	FBoxSphereBounds MeshBounds = StaticMesh->GetBounds();
	FVector MeshCenter = MeshBounds.Origin;
	float MeshRadius = MeshBounds.SphereRadius;

	// Check all sockets for reasonable transforms
	for (const UStaticMeshSocket* Socket : StaticMesh->Sockets)
	{
		if (Socket)
		{
			FVector SocketLocation = Socket->RelativeLocation;
			float DistanceFromCenter = FVector::Dist(SocketLocation, MeshCenter);
			
			// Check if socket is too far from mesh bounds
			if (DistanceFromCenter > MeshRadius + WarningDistance)
			{
				OutInvalidSocketNames.Add(Socket->SocketName.ToString());
			}
		}
	}

	return OutInvalidSocketNames.Num() > 0;
}

FString FStaticMeshSocketNamingRule::GenerateSocketNamingDescription(const UStaticMesh* StaticMesh, const TArray<FString>& InvalidNamingSockets, const TArray<FString>& InvalidTransformSockets, const FString& RequiredPrefix) const
{
	FString Description = FString::Printf(TEXT("Socket issues detected for %s: "), *StaticMesh->GetName());

	bool HasIssues = false;

	// Check naming issues
	if (InvalidNamingSockets.Num() > 0)
	{
		Description += FString::Printf(TEXT("Found %d socket(s) with invalid naming (must start with '%s'): "), InvalidNamingSockets.Num(), *RequiredPrefix);
		for (int32 i = 0; i < InvalidNamingSockets.Num(); ++i)
		{
			Description += InvalidNamingSockets[i];
			if (i < InvalidNamingSockets.Num() - 1)
			{
				Description += TEXT(", ");
			}
		}
		Description += TEXT(". ");
		HasIssues = true;
	}

	// Check transform issues
	if (InvalidTransformSockets.Num() > 0)
	{
		Description += FString::Printf(TEXT("Found %d socket(s) with invalid transforms (too far from mesh bounds): "), InvalidTransformSockets.Num());
		for (int32 i = 0; i < InvalidTransformSockets.Num(); ++i)
		{
			Description += InvalidTransformSockets[i];
			if (i < InvalidTransformSockets.Num() - 1)
			{
				Description += TEXT(", ");
			}
		}
		Description += TEXT(". ");
		HasIssues = true;
	}

	if (!HasIssues)
	{
		Description = FString::Printf(TEXT("Socket naming check failed for %s"), *StaticMesh->GetName());
	}

	return Description;
}

bool FStaticMeshSocketNamingRule::FixSocketIssues(UStaticMesh* StaticMesh, const FString& RequiredPrefix) const
{
	if (!StaticMesh)
	{
		return false;
	}

	UE_LOG(LogPipelineGuardian, Log, TEXT("Fixing socket issues for %s"), *StaticMesh->GetName());

	bool HasChanges = false;

	// Fix naming issues
	if (!RequiredPrefix.IsEmpty())
	{
		for (UStaticMeshSocket* Socket : StaticMesh->Sockets)
		{
			if (Socket && !Socket->SocketName.ToString().StartsWith(RequiredPrefix))
			{
				FString NewName = RequiredPrefix + Socket->SocketName.ToString();
				Socket->SocketName = FName(*NewName);
				UE_LOG(LogPipelineGuardian, Log, TEXT("Renamed socket to %s"), *NewName);
				HasChanges = true;
			}
		}
	}

	// Fix transform issues by moving sockets to reasonable positions
	FBoxSphereBounds MeshBounds = StaticMesh->GetBounds();
	FVector MeshCenter = MeshBounds.Origin;
	float MeshRadius = MeshBounds.SphereRadius;

	for (UStaticMeshSocket* Socket : StaticMesh->Sockets)
	{
		if (Socket)
		{
			FVector SocketLocation = Socket->RelativeLocation;
			float DistanceFromCenter = FVector::Dist(SocketLocation, MeshCenter);
			
			// If socket is too far, move it to the mesh surface
			if (DistanceFromCenter > MeshRadius * 1.5f)
			{
				FVector Direction = (SocketLocation - MeshCenter).GetSafeNormal();
				FVector NewLocation = MeshCenter + Direction * MeshRadius * 0.8f;
				Socket->RelativeLocation = NewLocation;
				UE_LOG(LogPipelineGuardian, Log, TEXT("Moved socket to reasonable position"));
				HasChanges = true;
			}
		}
	}

	if (HasChanges)
	{
		// Mark for rebuild
		StaticMesh->MarkPackageDirty();
		StaticMesh->PostEditChange();
	}

	UE_LOG(LogPipelineGuardian, Log, TEXT("Successfully fixed socket issues for %s"), *StaticMesh->GetName());
	return true;
}

bool FStaticMeshSocketNamingRule::CanSafelyFixSocketIssues(const UStaticMesh* StaticMesh) const
{
	if (!StaticMesh)
	{
		return false;
	}

	// Check if mesh has valid geometry
	if (!StaticMesh->GetRenderData() || StaticMesh->GetRenderData()->LODResources.Num() == 0)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("Cannot fix socket issues for %s: No valid geometry"), *StaticMesh->GetName());
		return false;
	}

	// Check if mesh has sockets to work with
	if (StaticMesh->Sockets.Num() == 0)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("Cannot fix socket issues for %s: No sockets to fix"), *StaticMesh->GetName());
		return false;
	}

	return true;
} 