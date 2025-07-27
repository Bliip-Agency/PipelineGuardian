// Copyright Epic Games, Inc. All Rights Reserved.

#include "PipelineGuardianCommands.h"

#define LOCTEXT_NAMESPACE "FPipelineGuardianModule"

void FPipelineGuardianCommands::RegisterCommands()
{
	UI_COMMAND(OpenPipelineGuardianWindowCommand, "Pipeline Guardian", "Opens the Pipeline Guardian analysis window.", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
