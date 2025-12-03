// Copyright Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphJsonSchema.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

FString FBlueprintGraphJsonSchema::GetCurrentSchemaVersion()
{
	return "1.0";
}

bool FBlueprintGraphJsonSchema::ValidateJson(const TSharedPtr<FJsonObject>& JsonData)
{
	if (!JsonData.IsValid())
	{
		return false;
	}

	// Check for metadata
	const TSharedPtr<FJsonObject>* MetadataObjectPtr;
	if (JsonData->TryGetObjectField("metadata", MetadataObjectPtr))
	{
		if (!ValidateMetadata(*MetadataObjectPtr))
		{
			return false;
		}
	}

	// Check for graph
	const TSharedPtr<FJsonObject>* GraphObjectPtr;
	if (!JsonData->TryGetObjectField("graph", GraphObjectPtr))
	{
		return false;
	}

	return ValidateGraph(*GraphObjectPtr);
}

bool FBlueprintGraphJsonSchema::ValidateMetadata(const TSharedPtr<FJsonObject>& MetadataObject)
{
	if (!MetadataObject.IsValid())
	{
		return false;
	}

	// Version is optional but recommended
	// Other metadata fields are optional

	return true;
}

bool FBlueprintGraphJsonSchema::ValidateGraph(const TSharedPtr<FJsonObject>& GraphObject)
{
	if (!GraphObject.IsValid())
	{
		return false;
	}

	// Validate nodes array (optional, but usually present)
	const TArray<TSharedPtr<FJsonValue>>* NodesArray;
	if (GraphObject->TryGetArrayField("nodes", NodesArray))
	{
		for (const TSharedPtr<FJsonValue>& NodeValue : *NodesArray)
		{
			const TSharedPtr<FJsonObject>* NodeObjectPtr;
			if (NodeValue->TryGetObject(NodeObjectPtr))
			{
				if (!ValidateNode(*NodeObjectPtr))
				{
					return false;
				}
			}
		}
	}

	// Validate connections array (optional, but usually present)
	const TArray<TSharedPtr<FJsonValue>>* ConnectionsArray;
	if (GraphObject->TryGetArrayField("connections", ConnectionsArray))
	{
		for (const TSharedPtr<FJsonValue>& ConnectionValue : *ConnectionsArray)
		{
			const TSharedPtr<FJsonObject>* ConnectionObjectPtr;
			if (ConnectionValue->TryGetObject(ConnectionObjectPtr))
			{
				if (!ValidateConnection(*ConnectionObjectPtr))
				{
					return false;
				}
			}
		}
	}

	return true;
}

bool FBlueprintGraphJsonSchema::ValidateNode(const TSharedPtr<FJsonObject>& NodeObject)
{
	if (!NodeObject.IsValid())
	{
		return false;
	}

	// Required fields: id, type
	FString NodeId, NodeType;
	if (!NodeObject->TryGetStringField("id", NodeId) ||
		!NodeObject->TryGetStringField("type", NodeType))
	{
		return false;
	}

	// Position is recommended but not strictly required
	// Other fields are optional

	return true;
}

bool FBlueprintGraphJsonSchema::ValidateConnection(const TSharedPtr<FJsonObject>& ConnectionObject)
{
	if (!ConnectionObject.IsValid())
	{
		return false;
	}

	// Required: from and to objects
	const TSharedPtr<FJsonObject>* FromObjectPtr;
	const TSharedPtr<FJsonObject>* ToObjectPtr;

	if (!ConnectionObject->TryGetObjectField("from", FromObjectPtr) ||
		!ConnectionObject->TryGetObjectField("to", ToObjectPtr))
	{
		return false;
	}

	// From object should have nodeId and pinName
	FString FromNodeId, FromPinName;
	if (!(*FromObjectPtr)->TryGetStringField("nodeId", FromNodeId) ||
		!(*FromObjectPtr)->TryGetStringField("pinName", FromPinName))
	{
		return false;
	}

	// To object should have nodeId and pinName
	FString ToNodeId, ToPinName;
	if (!(*ToObjectPtr)->TryGetStringField("nodeId", ToNodeId) ||
		!(*ToObjectPtr)->TryGetStringField("pinName", ToPinName))
	{
		return false;
	}

	return true;
}

bool FBlueprintGraphJsonSchema::MigrateJson(TSharedPtr<FJsonObject>& JsonData, int32 FromVersion)
{
	if (!JsonData.IsValid())
	{
		return false;
	}

	// TODO: Implement migration logic when schema versions change
	// For now, schema version 1.0, so no migration needed

	return true;
}

