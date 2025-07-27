// Copyright Epic Games, Inc. All Rights Reserved.

#include "Analysis/FPipelineGuardianProfile.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonReader.h"
#include "PipelineGuardian.h"

UPipelineGuardianProfile::UPipelineGuardianProfile()
	: ProfileName(TEXT("Default Profile"))
	, Description(TEXT("Default Pipeline Guardian profile"))
	, Version(1)
{
	InitializeDefaultRules();
}

void UPipelineGuardianProfile::InitializeDefaultRules()
{
	// Static Mesh Naming Rule
	FPipelineGuardianRuleConfig SMNamingRule(FName(TEXT("SM_Naming")), true);
	SMNamingRule.Parameters.Add(TEXT("NamingPattern"), TEXT("SM_*"));
	SetRuleConfig(SMNamingRule);

	// Static Mesh LOD Missing Rule
	FPipelineGuardianRuleConfig SMLODMissingRule(FName(TEXT("SM_LODMissing")), true);
	SetRuleConfig(SMLODMissingRule);

	// Static Mesh LOD Poly Reduction Rule
	FPipelineGuardianRuleConfig SMLODPolyReductionRule(FName(TEXT("SM_LODPolyReduction")), true);
	SMLODPolyReductionRule.Parameters.Add(TEXT("MinReductionPercentage"), TEXT("30.0"));
	SMLODPolyReductionRule.Parameters.Add(TEXT("WarningThreshold"), TEXT("20.0"));
	SMLODPolyReductionRule.Parameters.Add(TEXT("ErrorThreshold"), TEXT("10.0"));
	SetRuleConfig(SMLODPolyReductionRule);

	// Static Mesh UV Overlapping Rule
	FPipelineGuardianRuleConfig SMUVOverlappingRule(FName(TEXT("SM_UVOverlapping")), true);
	SMUVOverlappingRule.Parameters.Add(TEXT("Severity"), TEXT("Warning"));
	SMUVOverlappingRule.Parameters.Add(TEXT("CheckUVChannel0"), TEXT("true"));
	SMUVOverlappingRule.Parameters.Add(TEXT("CheckUVChannel1"), TEXT("true"));
	SMUVOverlappingRule.Parameters.Add(TEXT("CheckUVChannel2"), TEXT("false"));
	SMUVOverlappingRule.Parameters.Add(TEXT("CheckUVChannel3"), TEXT("false"));
	SMUVOverlappingRule.Parameters.Add(TEXT("TextureUVTolerance"), TEXT("0.001"));
	SMUVOverlappingRule.Parameters.Add(TEXT("LightmapUVTolerance"), TEXT("0.0005"));
	SMUVOverlappingRule.Parameters.Add(TEXT("TextureWarningThreshold"), TEXT("5.0"));
	SMUVOverlappingRule.Parameters.Add(TEXT("TextureErrorThreshold"), TEXT("15.0"));
	SMUVOverlappingRule.Parameters.Add(TEXT("LightmapWarningThreshold"), TEXT("2.0"));
	SMUVOverlappingRule.Parameters.Add(TEXT("LightmapErrorThreshold"), TEXT("8.0"));
	SMUVOverlappingRule.Parameters.Add(TEXT("AllowAutoFix"), TEXT("true"));
	SetRuleConfig(SMUVOverlappingRule);

	// Static Mesh Triangle Count Rule
	FPipelineGuardianRuleConfig SMTriangleCountRule(FName(TEXT("SM_TriangleCount")), true);
	SMTriangleCountRule.Parameters.Add(TEXT("Severity"), TEXT("Warning"));
	SMTriangleCountRule.Parameters.Add(TEXT("WarningThreshold"), TEXT("50000"));  // 50K triangles
	SMTriangleCountRule.Parameters.Add(TEXT("ErrorThreshold"), TEXT("100000"));   // 100K triangles
	SMTriangleCountRule.Parameters.Add(TEXT("AllowAutoFix"), TEXT("true"));
	SMTriangleCountRule.Parameters.Add(TEXT("PerformanceLODReductionTarget"), TEXT("60.0"));
	SetRuleConfig(SMTriangleCountRule);

	UE_LOG(LogPipelineGuardian, Log, TEXT("UPipelineGuardianProfile: Initialized with %d default rules"), RuleConfigs.Num());
}

const FPipelineGuardianRuleConfig* UPipelineGuardianProfile::GetRuleConfigPtr(FName RuleID) const
{
	return RuleConfigs.FindByPredicate([RuleID](const FPipelineGuardianRuleConfig& Config)
	{
		return Config.RuleID == RuleID;
	});
}

FPipelineGuardianRuleConfig UPipelineGuardianProfile::GetRuleConfig(FName RuleID) const
{
	const FPipelineGuardianRuleConfig* ConfigPtr = GetRuleConfigPtr(RuleID);
	if (ConfigPtr)
	{
		return *ConfigPtr;
	}
	
	// Return default config if not found
	return FPipelineGuardianRuleConfig();
}

void UPipelineGuardianProfile::SetRuleConfig(const FPipelineGuardianRuleConfig& RuleConfig)
{
	// Find existing config or add new one
	FPipelineGuardianRuleConfig* ExistingConfig = RuleConfigs.FindByPredicate([&RuleConfig](const FPipelineGuardianRuleConfig& Config)
	{
		return Config.RuleID == RuleConfig.RuleID;
	});

	if (ExistingConfig)
	{
		*ExistingConfig = RuleConfig;
	}
	else
	{
		RuleConfigs.Add(RuleConfig);
	}
}

bool UPipelineGuardianProfile::IsRuleEnabled(FName RuleID) const
{
	const FPipelineGuardianRuleConfig* Config = GetRuleConfigPtr(RuleID);
	return Config ? Config->bEnabled : false;
}

FString UPipelineGuardianProfile::GetRuleParameter(FName RuleID, const FString& ParameterName, const FString& DefaultValue) const
{
	const FPipelineGuardianRuleConfig* Config = GetRuleConfigPtr(RuleID);
	if (Config && Config->Parameters.Contains(ParameterName))
	{
		return Config->Parameters[ParameterName];
	}
	return DefaultValue;
}

FString UPipelineGuardianProfile::ExportToJSON() const
{
	TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
	
	// Profile metadata
	RootObject->SetStringField(TEXT("ProfileName"), ProfileName);
	RootObject->SetStringField(TEXT("Description"), Description);
	RootObject->SetNumberField(TEXT("Version"), Version);
	
	// Rule configurations
	TArray<TSharedPtr<FJsonValue>> RuleArray;
	for (const FPipelineGuardianRuleConfig& RuleConfig : RuleConfigs)
	{
		TSharedPtr<FJsonObject> RuleObject = MakeShareable(new FJsonObject);
		RuleObject->SetStringField(TEXT("RuleID"), RuleConfig.RuleID.ToString());
		RuleObject->SetBoolField(TEXT("Enabled"), RuleConfig.bEnabled);
		
		// Parameters
		TSharedPtr<FJsonObject> ParametersObject = MakeShareable(new FJsonObject);
		for (const auto& Param : RuleConfig.Parameters)
		{
			ParametersObject->SetStringField(Param.Key, Param.Value);
		}
		RuleObject->SetObjectField(TEXT("Parameters"), ParametersObject);
		
		RuleArray.Add(MakeShareable(new FJsonValueObject(RuleObject)));
	}
	RootObject->SetArrayField(TEXT("Rules"), RuleArray);
	
	// Serialize to string
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
	
	return OutputString;
}

bool UPipelineGuardianProfile::ImportFromJSON(const FString& JSONString)
{
	TSharedPtr<FJsonObject> RootObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JSONString);
	
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		UE_LOG(LogPipelineGuardian, Error, TEXT("Failed to parse JSON for profile import"));
		return false;
	}
	
	// Import profile metadata
	if (RootObject->HasField(TEXT("ProfileName")))
	{
		ProfileName = RootObject->GetStringField(TEXT("ProfileName"));
	}
	
	if (RootObject->HasField(TEXT("Description")))
	{
		Description = RootObject->GetStringField(TEXT("Description"));
	}
	
	if (RootObject->HasField(TEXT("Version")))
	{
		Version = RootObject->GetIntegerField(TEXT("Version"));
	}
	
	// Import rule configurations
	RuleConfigs.Empty();
	const TArray<TSharedPtr<FJsonValue>>* RuleArray;
	if (RootObject->TryGetArrayField(TEXT("Rules"), RuleArray))
	{
		for (const TSharedPtr<FJsonValue>& RuleValue : *RuleArray)
		{
			const TSharedPtr<FJsonObject>* RuleObject;
			if (RuleValue->TryGetObject(RuleObject))
			{
				FPipelineGuardianRuleConfig RuleConfig;
				
				FString RuleIDString;
				if ((*RuleObject)->TryGetStringField(TEXT("RuleID"), RuleIDString))
				{
					RuleConfig.RuleID = FName(*RuleIDString);
				}
				
				(*RuleObject)->TryGetBoolField(TEXT("Enabled"), RuleConfig.bEnabled);
				
				// Import parameters
				const TSharedPtr<FJsonObject>* ParametersObject;
				if ((*RuleObject)->TryGetObjectField(TEXT("Parameters"), ParametersObject))
				{
					for (const auto& Param : (*ParametersObject)->Values)
					{
						FString ParamValue;
						if (Param.Value->TryGetString(ParamValue))
						{
							RuleConfig.Parameters.Add(Param.Key, ParamValue);
						}
					}
				}
				
				RuleConfigs.Add(RuleConfig);
			}
		}
	}
	
	UE_LOG(LogPipelineGuardian, Log, TEXT("Successfully imported profile '%s' with %d rules"), *ProfileName, RuleConfigs.Num());
	return true;
} 