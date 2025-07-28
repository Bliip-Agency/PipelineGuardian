// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"
#include "HAL/Platform.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPipelineGuardian, Log, All);

class FToolBarBuilder;
class FMenuBuilder;

/**
 * Pipeline Guardian Plugin Constants
 * Centralized configuration values used throughout the plugin
 */
namespace PipelineGuardianConstants
{
	// Performance thresholds
	constexpr int32 MAX_TRIANGLE_COUNT_FOR_AUTO_FIX = 500000;
	constexpr int32 MAX_TRIANGLE_COUNT_FOR_VERTEX_COLOR_CHECK = 100000;
	constexpr int32 MAX_TRIANGLE_COUNT_FOR_COLLISION_GENERATION = 50000;
	constexpr int32 MAX_TRIANGLE_COUNT_FOR_LIGHTMAP_RESOLUTION = 1000000;
	constexpr int32 MAX_TRIANGLE_COUNT_FOR_NANITE_ENABLE = 1000000;
	
	// Scale thresholds
	constexpr float MIN_SCALE_THRESHOLD = 0.001f;
	constexpr float MAX_SCALE_THRESHOLD = 1000.0f;
	constexpr float ZERO_SCALE_THRESHOLD = 0.01f;
	
	// UV overlap tolerances
	constexpr float MIN_UV_OVERLAP_TOLERANCE = 0.0001f;
	constexpr float MAX_UV_OVERLAP_TOLERANCE = 0.01f;
	constexpr float PRIMARY_UV_TOLERANCE = 0.001f;
	
	// Distance thresholds
	constexpr float MAX_PIVOT_DISTANCE = 1000.0f;
	constexpr float MAX_SURFACE_AREA = 100000.0f;
	
	// Default values
	constexpr float DEFAULT_LOD_REDUCTION_PERCENTAGE = 0.5f;
	constexpr float DEFAULT_TRIANGLE_COUNT_BASE = 50000.0f;
	constexpr float DEFAULT_NON_UNIFORM_SCALE_RATIO = 2.0f;
	
	// Clamp values
	constexpr float MIN_LOD_REDUCTION_CLAMP = 0.01f;
	constexpr float MAX_LOD_REDUCTION_CLAMP = 1.0f;
}

class FPipelineGuardianModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();
	
private:

	void RegisterMenus();

	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

private:
	TSharedPtr<class FUICommandList> PluginCommands;
};
