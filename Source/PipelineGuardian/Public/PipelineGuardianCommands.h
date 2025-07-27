// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Framework/Commands/Commands.h"
#include "PipelineGuardianStyle.h"

class FPipelineGuardianCommands : public TCommands<FPipelineGuardianCommands>
{
public:

	FPipelineGuardianCommands()
		: TCommands<FPipelineGuardianCommands>(TEXT("PipelineGuardian"), NSLOCTEXT("Contexts", "PipelineGuardian", "PipelineGuardian Plugin"), NAME_None, FPipelineGuardianStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPipelineGuardianWindowCommand;
};