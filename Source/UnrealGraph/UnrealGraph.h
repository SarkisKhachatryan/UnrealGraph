// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * UnrealGraph module interface
 */
class FUnrealGraphModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** Command list for menu actions */
	TSharedPtr<class FUICommandList> CommandList;

	/** Register editor commands and menus */
	void RegisterMenus();

	/** Register console commands for testing */
	void RegisterConsoleCommands();
	
	/** Test serialization function */
	void TestSerialization();

	/** Register Blueprint editor menus */
	void RegisterBlueprintEditorMenus();
	
	/** Handler for Copy as JSON */
	void OnCopyAsJSON();
	
	/** Handler for Paste from JSON */
	void OnPasteFromJSON();
	
	/** Get the currently focused Blueprint graph */
	UEdGraph* GetFocusedBlueprintGraph() const;
};

