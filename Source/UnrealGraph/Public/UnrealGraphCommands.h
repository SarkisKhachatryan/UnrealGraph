// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "UnrealGraphStyle.h"

class FUnrealGraphCommands : public TCommands<FUnrealGraphCommands>
{
public:
	FUnrealGraphCommands();

	virtual void RegisterCommands() override;

public:
	/** Copy graph as JSON */
	TSharedPtr<FUICommandInfo> CopyAsJSON;
	
	/** Paste graph from JSON */
	TSharedPtr<FUICommandInfo> PasteFromJSON;
};

