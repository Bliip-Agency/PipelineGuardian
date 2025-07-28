#include "FStaticMeshCollisionMissingRule.h"
#include "Analysis/FAssetAnalysisResult.h"
#include "FPipelineGuardianSettings.h"
#include "Engine/StaticMesh.h"
#include "Engine/CollisionProfile.h"
#include "PhysicsEngine/BodySetup.h"
#include "Misc/MessageDialog.h"
#include "Framework/Application/SlateApplication.h"
#include "PipelineGuardian.h"

FStaticMeshCollisionMissingRule::FStaticMeshCollisionMissingRule()
{
	UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshCollisionMissingRule initialized"));
}

bool FStaticMeshCollisionMissingRule::Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults)
{
	UStaticMesh* StaticMesh = Cast<UStaticMesh>(Asset);
	if (!StaticMesh)
	{
		return false;
	}

	const UPipelineGuardianSettings* Settings = GetDefault<UPipelineGuardianSettings>();
	if (!Settings)
	{
		UE_LOG(LogPipelineGuardian, Error, TEXT("FStaticMeshCollisionMissingRule: Could not get Pipeline Guardian settings"));
		return false;
	}

	// Check if rule is enabled
	if (!Settings->bEnableStaticMeshCollisionMissingRule)
	{
		return false;
	}

	// Check for missing collision
	if (HasMissingCollision(StaticMesh))
	{
		EAssetIssueSeverity Severity = Settings->CollisionMissingIssueSeverity;

		FAssetAnalysisResult Result;
		Result.RuleID = GetRuleID();
		Result.Asset = FAssetData(StaticMesh);
		Result.Severity = Severity;
		Result.Description = FText::FromString(GenerateMissingCollisionDescription(StaticMesh, Severity));
		Result.FilePath = FText::FromString(StaticMesh->GetPackage()->GetName());

			// Add fix action if enabled and safe
	bool bCanSafelyFix = CanSafelyGenerateCollision(StaticMesh);
	UE_LOG(LogPipelineGuardian, Log, TEXT("Collision Missing Rule: Auto-fix enabled=%s, can safely fix=%s for %s"), 
		Settings->bAllowCollisionMissingAutoFix ? TEXT("true") : TEXT("false"),
		bCanSafelyFix ? TEXT("true") : TEXT("false"),
		*StaticMesh->GetName());
		
	if (Settings->bAllowCollisionMissingAutoFix && bCanSafelyFix)
	{
		Result.FixAction.BindLambda([StaticMesh, this]()
		{
			if (GenerateCollision(StaticMesh))
			{
				FText SuccessMessage = FText::FromString(FString::Printf(TEXT("Successfully generated collision for '%s'"), *StaticMesh->GetName()));
				FMessageDialog::Open(EAppMsgType::Ok, SuccessMessage, FText::FromString(TEXT("Collision Generation Success")));
			}
			else
			{
				FText ErrorMessage = FText::FromString(FString::Printf(TEXT("Failed to generate collision for '%s'. Please check the mesh manually."), *StaticMesh->GetName()));
				FMessageDialog::Open(EAppMsgType::Ok, ErrorMessage, FText::FromString(TEXT("Collision Generation Error")));
			}
		});
	}
	else
	{
		// Add a note to the description about why auto-fix is disabled
		FString CurrentDesc = Result.Description.ToString();
		CurrentDesc += TEXT(" (Auto-fix disabled - check mesh complexity or settings)");
		Result.Description = FText::FromString(CurrentDesc);
	}

		OutResults.Add(Result);

		UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshCollisionMissingRule::Check: Found missing collision in %s"), *StaticMesh->GetName());

		return true;
	}

	return false;
}

FName FStaticMeshCollisionMissingRule::GetRuleID() const
{
	return TEXT("SM_CollisionMissing");
}

FText FStaticMeshCollisionMissingRule::GetRuleDescription() const
{
	return FText::FromString(TEXT("Detects static meshes that are missing collision geometry, which can cause physics issues and gameplay problems."));
}

bool FStaticMeshCollisionMissingRule::HasMissingCollision(const UStaticMesh* StaticMesh) const
{
	if (!StaticMesh)
	{
		return false;
	}

	// Check if the mesh has a body setup
	UBodySetup* BodySetup = StaticMesh->GetBodySetup();
	if (!BodySetup)
	{
		return true; // No body setup means no collision
	}

	// Check if the body setup has any collision primitives
	if (BodySetup->AggGeom.GetElementCount() == 0)
	{
		return true; // No collision primitives
	}

	// Check if the collision profile is set to "NoCollision"
	if (BodySetup->DefaultInstance.GetCollisionProfileName() == UCollisionProfile::NoCollision_ProfileName)
	{
		return true; // Explicitly set to no collision
	}

	return false;
}

FString FStaticMeshCollisionMissingRule::GenerateMissingCollisionDescription(const UStaticMesh* StaticMesh, EAssetIssueSeverity Severity) const
{
	FString SeverityText = (Severity == EAssetIssueSeverity::Error) ? TEXT("CRITICAL") : TEXT("WARNING");
	
	return FString::Printf(
		TEXT("%s: Static mesh '%s' is missing collision geometry. ")
		TEXT("Missing collision can cause physics issues, navigation problems, and gameplay bugs. ")
		TEXT("Generate collision geometry to ensure proper physics simulation and gameplay functionality."),
		*SeverityText,
		*StaticMesh->GetName()
	);
}

bool FStaticMeshCollisionMissingRule::GenerateCollision(UStaticMesh* StaticMesh) const
{
	if (!StaticMesh)
	{
		return false;
	}

	UE_LOG(LogPipelineGuardian, Log, TEXT("Generating collision for %s"), *StaticMesh->GetName());

	// Get or create body setup
	UBodySetup* BodySetup = StaticMesh->GetBodySetup();
	if (!BodySetup)
	{
		BodySetup = NewObject<UBodySetup>(StaticMesh, NAME_None, RF_Transactional);
		StaticMesh->SetBodySetup(BodySetup);
	}

	// Set default collision profile
	BodySetup->DefaultInstance.SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	// Enable auto-convex collision generation
	BodySetup->bGenerateMirroredCollision = false;
	BodySetup->bDoubleSidedGeometry = false;

	// Generate collision from mesh
	BodySetup->CreatePhysicsMeshes();

	// Force a rebuild of the collision
	StaticMesh->Build(false);
	
	// Mark the asset as dirty
	StaticMesh->MarkPackageDirty();
	StaticMesh->PostEditChange();

	// Verify collision was created
	if (BodySetup->AggGeom.GetElementCount() > 0)
	{
		UE_LOG(LogPipelineGuardian, Log, TEXT("Successfully generated collision for %s with %d primitives"), 
			*StaticMesh->GetName(), BodySetup->AggGeom.GetElementCount());
		return true;
	}
	else
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("Auto-generation failed for %s - creating fallback box collision"), 
			*StaticMesh->GetName());
		
		// Create a fallback box collision based on mesh bounds
		if (StaticMesh->GetRenderData() && StaticMesh->GetRenderData()->LODResources.Num() > 0)
		{
			// Get mesh bounds from the static mesh itself
			FBoxSphereBounds Bounds = StaticMesh->GetBounds();
			
			// Create a box collision that fits the mesh
			FKBoxElem BoxElem;
			BoxElem.Center = Bounds.Origin;
			BoxElem.Rotation = FRotator::ZeroRotator;
			BoxElem.X = Bounds.BoxExtent.X;
			BoxElem.Y = Bounds.BoxExtent.Y;
			BoxElem.Z = Bounds.BoxExtent.Z;
			
			BodySetup->AggGeom.BoxElems.Add(BoxElem);
			
			UE_LOG(LogPipelineGuardian, Log, TEXT("Created fallback box collision for %s"), *StaticMesh->GetName());
			return true;
		}
		
		return false;
	}
}

bool FStaticMeshCollisionMissingRule::CanSafelyGenerateCollision(const UStaticMesh* StaticMesh) const
{
	if (!StaticMesh)
	{
		return false;
	}

	// Check if mesh has valid geometry
	if (!StaticMesh->GetRenderData() || StaticMesh->GetRenderData()->LODResources.Num() == 0)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("Cannot generate collision for %s: No valid geometry"), *StaticMesh->GetName());
		return false;
	}

	// Check if mesh is not too complex for auto-generation
	const FStaticMeshLODResources* LODResource = &StaticMesh->GetRenderData()->LODResources[0];
	int32 TriangleCount = LODResource->GetNumTriangles();
	
	// Don't auto-generate for very complex meshes (more than 50k triangles for testing)
	if (TriangleCount > PipelineGuardianConstants::MAX_TRIANGLE_COUNT_FOR_COLLISION_GENERATION)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("Cannot auto-generate collision for %s: Too complex (%d triangles)"), 
			*StaticMesh->GetName(), TriangleCount);
		return false;
	}

	return true;
} 