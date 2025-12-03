// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * JSON schema definition and validation for Blueprint graph JSON format
 */
class FBlueprintGraphJsonSchema
{
public:
	/**
	 * Get the current schema version
	 * @return Schema version string
	 */
	static FString GetCurrentSchemaVersion();

	/**
	 * Validate JSON against the schema
	 * @param JsonData The JSON object to validate
	 * @return True if JSON structure is valid
	 */
	static bool ValidateJson(const TSharedPtr<FJsonObject>& JsonData);

	/**
	 * Migrate JSON from an older schema version
	 * @param JsonData The JSON object to migrate
	 * @param FromVersion The version to migrate from
	 * @return True if migration succeeded
	 */
	static bool MigrateJson(TSharedPtr<FJsonObject>& JsonData, int32 FromVersion);

private:
	/**
	 * Validate metadata section
	 */
	static bool ValidateMetadata(const TSharedPtr<FJsonObject>& MetadataObject);

	/**
	 * Validate graph section
	 */
	static bool ValidateGraph(const TSharedPtr<FJsonObject>& GraphObject);

	/**
	 * Validate node structure
	 */
	static bool ValidateNode(const TSharedPtr<FJsonObject>& NodeObject);

	/**
	 * Validate connection structure
	 */
	static bool ValidateConnection(const TSharedPtr<FJsonObject>& ConnectionObject);
};

