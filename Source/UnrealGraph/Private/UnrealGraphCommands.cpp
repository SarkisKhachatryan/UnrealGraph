// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnrealGraphCommands.h"
#include "UnrealGraphStyle.h"

#define LOCTEXT_NAMESPACE "FUnrealGraphCommands"

FUnrealGraphCommands::FUnrealGraphCommands()
	: TCommands<FUnrealGraphCommands>(
		TEXT("UnrealGraph"),
		NSLOCTEXT("Contexts", "UnrealGraph", "Unreal Graph Plugin"),
		NAME_None,
		FUnrealGraphStyle::GetStyleSetName()
	)
{
}

void FUnrealGraphCommands::RegisterCommands()
{
	UI_COMMAND(CopyAsJSON, "Copy Graph as JSON", "Copy the selected Blueprint graph as JSON", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::C));
	UI_COMMAND(PasteFromJSON, "Paste Graph from JSON", "Paste a Blueprint graph from JSON", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::V));
}

#undef LOCTEXT_NAMESPACE

