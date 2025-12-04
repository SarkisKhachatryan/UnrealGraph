// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"

/**
 * Logger utility for UnrealGraph plugin
 * Writes detailed logs to file for debugging and analysis
 */
class FUnrealGraphLogger
{
public:
	/**
	 * Initialize the logger - opens log file
	 * @param LogFileName Name of the log file (without extension)
	 */
	static void Initialize(const FString& LogFileName = TEXT("UnrealGraph_Debug"));

	/**
	 * Close the logger and flush any remaining content
	 */
	static void Shutdown();

	/**
	 * Log a message to the log file
	 * @param Message The message to log
	 */
	static void Log(const FString& Message);

	/**
	 * Log a formatted message
	 * @param Format Format string (supports %s, %d, %f, etc.)
	 * @param ... Arguments for formatting
	 */
	static void LogFormatted(const TCHAR* Format, ...);

	/**
	 * Log a section header
	 * @param SectionName Name of the section
	 */
	static void LogSection(const FString& SectionName);

	/**
	 * Log detailed node information
	 * @param Node The node to log information about
	 */
	static void LogNodeDetails(class UEdGraphNode* Node);

	/**
	 * Log detailed property information
	 * @param PropertyName Name of the property
	 * @param PropertyValue Value as string
	 */
	static void LogProperty(const FString& PropertyName, const FString& PropertyValue);

	/**
	 * Check if logger is initialized
	 */
	static bool IsInitialized() { return bIsInitialized; }

private:
	static bool bIsInitialized;
	static FString LogFilePath;
	static TArray<FString> LogBuffer;

	/**
	 * Flush log buffer to file
	 */
	static void FlushLogBuffer();
};

