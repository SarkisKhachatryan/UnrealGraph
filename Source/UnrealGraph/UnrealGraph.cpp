// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnrealGraph.h"
#include "UnrealGraphCommands.h"
#include "UnrealGraphStyle.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "FUnrealGraphModule"

void FUnrealGraphModule::StartupModule()
{
	// Initialize style
	FUnrealGraphStyle::Initialize();
	
	// Register commands
	FUnrealGraphCommands::Register();
	
	// Register menu extensions
	RegisterMenus();
}

void FUnrealGraphModule::ShutdownModule()
{
	// Unregister menu extensions
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	
	// Unregister commands
	FUnrealGraphCommands::Unregister();
	
	// Shutdown style
	FUnrealGraphStyle::Shutdown();
}

void FUnrealGraphModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);
	
	// Register context menu extensions
	// Implementation will be added in Phase 4: Editor Integration
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUnrealGraphModule, UnrealGraph)

