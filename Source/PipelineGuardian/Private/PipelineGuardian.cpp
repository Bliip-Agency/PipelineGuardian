// Copyright Epic Games, Inc. All Rights Reserved.

#include "PipelineGuardian.h"
#include "PipelineGuardianStyle.h"
#include "PipelineGuardianCommands.h"
#include "UI/SPipelineGuardianWindow.h"
#include "FPipelineGuardianSettings.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"
#include "Modules/ModuleManager.h"
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "ISettingsContainer.h"

DEFINE_LOG_CATEGORY(LogPipelineGuardian);

static const FName PipelineGuardianTabName("PipelineGuardian");

#define LOCTEXT_NAMESPACE "FPipelineGuardianModule"

void FPipelineGuardianModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FPipelineGuardianStyle::Initialize();
	FPipelineGuardianStyle::ReloadTextures();

	FPipelineGuardianCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FPipelineGuardianCommands::Get().OpenPipelineGuardianWindowCommand,
		FExecuteAction::CreateRaw(this, &FPipelineGuardianModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FPipelineGuardianModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(PipelineGuardianTabName, FOnSpawnTab::CreateRaw(this, &FPipelineGuardianModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FPipelineGuardianTabTitle", "PipelineGuardian"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	UE_LOG(LogPipelineGuardian, Warning, TEXT("PipelineGuardianModule Startup: Attempting to access settings..."));
	UPipelineGuardianSettings* Settings = GetMutableDefault<UPipelineGuardianSettings>();
	if (Settings)
	{
		UE_LOG(LogPipelineGuardian, Warning, TEXT("PipelineGuardianSettings loaded successfully (mutable). MasterSwitch is: %s"), Settings->bMasterSwitch_EnableAnalysis ? TEXT("Enabled") : TEXT("Disabled"));
		
		Settings->Modify(); 
		Settings->SaveConfig(); 
		UE_LOG(LogPipelineGuardian, Warning, TEXT("Called Modify() and SaveConfig() on PipelineGuardianSettings."));
	}
	else
	{
		UE_LOG(LogPipelineGuardian, Error, TEXT("Failed to load PipelineGuardianSettings (mutable)!"));
	}

	// Manual Settings Registration Test - Ensure this is commented out for automatic discovery test
	/* ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if (SettingsModule)
	{
		SettingsModule->RegisterSettings("Project", // Container Name
			"PipelineGuardianTestCategory",          // Category Name (under which sections are grouped)
			"PipelineGuardianTestSection",           // Section Name (the actual entry in the list)
			LOCTEXT("PipelineGuardianTestDisplayName", "Pipeline Guardian (Manual Test)"),
			LOCTEXT("PipelineGuardianSettingsDescription_Manual", "Manually registered Pipeline Guardian settings."),
			GetMutableDefault<UPipelineGuardianSettings>()
		);
		UE_LOG(LogPipelineGuardian, Warning, TEXT("Attempted manual registration of settings UI section for 'PipelineGuardianTestSection'."));
	}*/
}

void FPipelineGuardianModule::ShutdownModule()
{
	// Manual Settings Unregistration Test - Ensure this is commented out
	/* ISettingsModule* SettingsModulePtr = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if (SettingsModulePtr)
	{
		SettingsModulePtr->UnregisterSettings("Project", "PipelineGuardianTestCategory", "PipelineGuardianTestSection");
		UE_LOG(LogPipelineGuardian, Warning, TEXT("Attempted manual unregistration of settings UI section for 'PipelineGuardianTestSection'."));
	}*/

	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FPipelineGuardianStyle::Shutdown();

	FPipelineGuardianCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(PipelineGuardianTabName);
}

TSharedRef<SDockTab> FPipelineGuardianModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SPipelineGuardianWindow)
		];
}

void FPipelineGuardianModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(PipelineGuardianTabName);
}

void FPipelineGuardianModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FPipelineGuardianCommands::Get().OpenPipelineGuardianWindowCommand, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FPipelineGuardianCommands::Get().OpenPipelineGuardianWindowCommand));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FPipelineGuardianModule, PipelineGuardian)