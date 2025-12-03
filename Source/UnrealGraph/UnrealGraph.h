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
	/** Handle to the menu extension */
	TSharedPtr<class FExtensibilityManager> MenuExtensibilityManager;
	TSharedPtr<class FExtensibilityManager> ToolBarExtensibilityManager;

	/** Register editor commands and menus */
	void RegisterMenus();
};

