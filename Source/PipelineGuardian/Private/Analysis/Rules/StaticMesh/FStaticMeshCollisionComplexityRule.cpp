#include "FStaticMeshCollisionComplexityRule.h"
#include "Analysis/FAssetAnalysisResult.h"
#include "FPipelineGuardianSettings.h"
#include "Engine/StaticMesh.h"
#include "Engine/CollisionProfile.h"
#include "PhysicsEngine/BodySetup.h"
#include "Misc/MessageDialog.h"
#include "Framework/Application/SlateApplication.h"
#include "PipelineGuardian.h"

FStaticMeshCollisionComplexityRule::FStaticMeshCollisionComplexityRule()
{
	UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshCollisionComplexityRule initialized"));
}

bool FStaticMeshCollisionComplexityRule::Check(UObject* Asset, const UPipelineGuardianProfile* Profile, TArray<FAssetAnalysisResult>& OutResults)
{
	UStaticMesh* StaticMesh = Cast<UStaticMesh>(Asset);
	if (!StaticMesh)
	{
		return false;
	}

	const UPipelineGuardianSettings* Settings = GetDefault<UPipelineGuardianSettings>();
	if (!Settings)
	{
		UE_LOG(LogPipelineGuardian, Error, TEXT("FStaticMeshCollisionComplexityRule: Could not get Pipeline Guardian settings"));
		return false;
	}

	// Check if rule is enabled
	if (!Settings->bEnableStaticMeshCollisionComplexityRule)
	{
		return false;
	}

	int32 PrimitiveCount = 0;
	bool UseComplexAsSimple = false;

	// Check for complex collision
	if (HasComplexCollision(StaticMesh, PrimitiveCount, UseComplexAsSimple))
	{
		// Determine severity based on primitive count and settings
		EAssetIssueSeverity Severity = EAssetIssueSeverity::Info;
		
		if (PrimitiveCount >= Settings->CollisionComplexityErrorThreshold)
		{
			Severity = EAssetIssueSeverity::Error;
		}
		else if (PrimitiveCount >= Settings->CollisionComplexityWarningThreshold)
		{
			Severity = EAssetIssueSeverity::Warning;
		}

		// Also check UseComplexAsSimple setting
		if (UseComplexAsSimple && Settings->bTreatUseComplexAsSimpleAsError)
		{
			Severity = EAssetIssueSeverity::Error;
		}

		if (Severity != EAssetIssueSeverity::Info)
		{
			FAssetAnalysisResult Result;
			Result.RuleID = GetRuleID();
			Result.Asset = FAssetData(StaticMesh);
			Result.Severity = Severity;
			Result.Description = FText::FromString(GenerateCollisionComplexityDescription(StaticMesh, PrimitiveCount, UseComplexAsSimple, Severity));
			Result.FilePath = FText::FromString(StaticMesh->GetPackage()->GetName());

			// Add fix action if enabled and safe
			if (Settings->bAllowCollisionComplexityAutoFix && CanSafelySimplifyCollision(StaticMesh))
			{
				Result.FixAction.BindLambda([StaticMesh, this]()
				{
					if (SimplifyCollision(StaticMesh))
					{
						FText SuccessMessage = FText::FromString(FString::Printf(TEXT("Successfully simplified collision for '%s'"), *StaticMesh->GetName()));
						FMessageDialog::Open(EAppMsgType::Ok, SuccessMessage, FText::FromString(TEXT("Collision Simplification Success")));
					}
					else
					{
						FText ErrorMessage = FText::FromString(FString::Printf(TEXT("Failed to simplify collision for '%s'. Please check the mesh manually."), *StaticMesh->GetName()));
						FMessageDialog::Open(EAppMsgType::Ok, ErrorMessage, FText::FromString(TEXT("Collision Simplification Error")));
					}
				});
			}

			OutResults.Add(Result);

			UE_LOG(LogPipelineGuardian, Log, TEXT("FStaticMeshCollisionComplexityRule::Check: Found complex collision in %s (%d primitives, UseComplexAsSimple: %s)"), 
				*StaticMesh->GetName(), PrimitiveCount, UseComplexAsSimple ? TEXT("true") : TEXT("false"));

			return true;
		}
	}

	return false;
}

FName FStaticMeshCollisionComplexityRule::GetRuleID() const
{
	return TEXT("SM_CollisionComplexity");
}

FText FStaticMeshCollisionComplexityRule::GetRuleDescription() const
{
	return FText::FromString(TEXT("Detects static meshes with overly complex collision geometry that can cause performance issues and physics problems."));
}

bool FStaticMeshCollisionComplexityRule::HasComplexCollision(const UStaticMesh* StaticMesh, int32& OutPrimitiveCount, bool& OutUseComplexAsSimple) const
{
	if (!StaticMesh)
	{
		return false;
	}

	// Get body setup
	UBodySetup* BodySetup = StaticMesh->GetBodySetup();
	if (!BodySetup)
	{
		return false; // No collision to analyze
	}

	// Count collision primitives
	OutPrimitiveCount = BodySetup->AggGeom.GetElementCount();

	// Check UseComplexAsSimple setting - in UE 5.5 this might be accessed differently
	// For now, we'll check if the collision is using complex collision as simple
	// This is a simplified check - we'll look for complex collision primitives
	OutUseComplexAsSimple = false; // Default to false, we'll implement proper detection later

	// Consider collision complex if:
	// 1. Too many primitives, or
	// 2. UseComplexAsSimple is enabled
	return (OutPrimitiveCount > 0) && (OutPrimitiveCount > 10 || OutUseComplexAsSimple);
}

FString FStaticMeshCollisionComplexityRule::GenerateCollisionComplexityDescription(const UStaticMesh* StaticMesh, int32 PrimitiveCount, bool UseComplexAsSimple, EAssetIssueSeverity Severity) const
{
	FString SeverityText = (Severity == EAssetIssueSeverity::Error) ? TEXT("CRITICAL") : TEXT("WARNING");
	
	FString ComplexityInfo = FString::Printf(TEXT("Collision has %d primitives"), PrimitiveCount);
	if (UseComplexAsSimple)
	{
		ComplexityInfo += TEXT(" and uses complex collision as simple");
	}
	
	return FString::Printf(
		TEXT("%s: Static mesh '%s' has overly complex collision geometry. %s. ")
		TEXT("Complex collision can cause performance issues, physics simulation problems, and memory overhead. ")
		TEXT("Simplify collision geometry to improve performance and stability."),
		*SeverityText,
		*StaticMesh->GetName(),
		*ComplexityInfo
	);
}

bool FStaticMeshCollisionComplexityRule::SimplifyCollision(UStaticMesh* StaticMesh) const
{
	if (!StaticMesh)
	{
		return false;
	}

	UE_LOG(LogPipelineGuardian, Log, TEXT("Simplifying collision for %s"), *StaticMesh->GetName());

	// Get body setup
	UBodySetup* BodySetup = StaticMesh->GetBodySetup();
	if (!BodySetup)
	{
		return false;
	}

	// Clear existing collision primitives
	BodySetup->AggGeom.EmptyElements();

	// Disable UseComplexAsSimple - in UE 5.5 this might be set differently
	// For now, we'll just clear the complex collision primitives
	// TODO: Implement proper UseComplexAsSimple handling for UE 5.5

	// Enable auto-convex collision generation
	BodySetup->bGenerateMirroredCollision = false;
	BodySetup->bDoubleSidedGeometry = false;

	// Generate simplified collision (auto-convex hull)
	BodySetup->CreatePhysicsMeshes();

	// Force a rebuild of the collision
	StaticMesh->Build(false);

	// Mark the asset as dirty
	StaticMesh->MarkPackageDirty();
	StaticMesh->PostEditChange();

	// Verify collision was created
	if (BodySetup->AggGeom.GetElementCount() > 0)
	{
		UE_LOG(LogPipelineGuardian, Log, TEXT("Successfully simplified collision for %s with %d primitives"), 
			*StaticMesh->GetName(), BodySetup->AggGeom.GetElementCount());
		return true;
	}
	else
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("Failed to simplify collision for %s - no primitives created"), 
			*StaticMesh->GetName());
		return false;
	}
}

bool FStaticMeshCollisionComplexityRule::CanSafelySimplifyCollision(const UStaticMesh* StaticMesh) const
{
	if (!StaticMesh)
	{
		return false;
	}

	// Check if mesh has valid geometry
	if (!StaticMesh->GetRenderData() || StaticMesh->GetRenderData()->LODResources.Num() == 0)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("Cannot simplify collision for %s: No valid geometry"), *StaticMesh->GetName());
		return false;
	}

	// Check if mesh has collision to simplify
	UBodySetup* BodySetup = StaticMesh->GetBodySetup();
	if (!BodySetup || BodySetup->AggGeom.GetElementCount() == 0)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("Cannot simplify collision for %s: No collision to simplify"), *StaticMesh->GetName());
		return false;
	}

	return true;
} 