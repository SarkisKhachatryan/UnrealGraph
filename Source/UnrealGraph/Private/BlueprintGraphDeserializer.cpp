// Copyright Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphDeserializer.h"
#include "BlueprintGraphJsonSchema.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "ScopedTransaction.h"

bool FBlueprintGraphDeserializer::DeserializeGraph(UEdGraph* Graph, const TSharedPtr<FJsonObject>& JsonData)
{
	if (!Graph || !JsonData.IsValid())
	{
		return false;
	}

	// Validate JSON schema
	if (!ValidateJsonSchema(JsonData))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid JSON schema"));
		return false;
	}

	// Begin transaction for undo/redo support
	FScopedTransaction Transaction(NSLOCTEXT("UnrealGraph", "PasteGraph", "Paste Graph from JSON"));

	// Get graph data
	const TSharedPtr<FJsonObject>* GraphObjectPtr;
	if (!JsonData->TryGetObjectField("graph", GraphObjectPtr))
	{
		UE_LOG(LogTemp, Error, TEXT("Missing 'graph' field in JSON"));
		return false;
	}

	TSharedPtr<FJsonObject> GraphObject = *GraphObjectPtr;

	// Create nodes
	const TArray<TSharedPtr<FJsonValue>>* NodesArray;
	if (GraphObject->TryGetArrayField("nodes", NodesArray))
	{
		for (const TSharedPtr<FJsonValue>& NodeValue : *NodesArray)
		{
			const TSharedPtr<FJsonObject>* NodeObjectPtr;
			if (NodeValue->TryGetObject(NodeObjectPtr))
			{
				CreateNodeFromJson(Graph, *NodeObjectPtr);
			}
		}
	}

	// Create connections
	const TArray<TSharedPtr<FJsonValue>>* ConnectionsArray;
	if (GraphObject->TryGetArrayField("connections", ConnectionsArray))
	{
		CreateConnectionsFromJson(Graph, *ConnectionsArray);
	}

	// Mark Blueprint as modified
	if (UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph))
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	}

	return true;
}

UEdGraphNode* FBlueprintGraphDeserializer::CreateNodeFromJson(UEdGraph* Graph, const TSharedPtr<FJsonObject>& NodeData)
{
	if (!Graph || !NodeData.IsValid())
	{
		return nullptr;
	}

	// Get node type
	FString NodeType;
	if (!NodeData->TryGetStringField("type", NodeType))
	{
		UE_LOG(LogTemp, Warning, TEXT("Missing 'type' field in node data"));
		return nullptr;
	}

	// TODO: Implement node creation based on type
	// For now, this is a placeholder that will be implemented in Phase 3
	// We need to:
	// 1. Map JSON node type to UClass
	// 2. Use FGraphNodeCreator to create the node
	// 3. Set node properties
	// 4. Set node position
	// 5. Allocate default pins

	UE_LOG(LogTemp, Warning, TEXT("CreateNodeFromJson not yet fully implemented for node type: %s"), *NodeType);

	return nullptr;
}

bool FBlueprintGraphDeserializer::CreateConnectionsFromJson(UEdGraph* Graph, const TArray<TSharedPtr<FJsonValue>>& ConnectionsArray)
{
	if (!Graph)
	{
		return false;
	}

	for (const TSharedPtr<FJsonValue>& ConnectionValue : ConnectionsArray)
	{
		const TSharedPtr<FJsonObject>* ConnectionObjectPtr;
		if (!ConnectionValue->TryGetObject(ConnectionObjectPtr))
		{
			continue;
		}

		TSharedPtr<FJsonObject> ConnectionObject = *ConnectionObjectPtr;

		// Get "from" connection point
		const TSharedPtr<FJsonObject>* FromObjectPtr;
		const TSharedPtr<FJsonObject>* ToObjectPtr;
		
		if (!ConnectionObject->TryGetObjectField("from", FromObjectPtr) ||
			!ConnectionObject->TryGetObjectField("to", ToObjectPtr))
		{
			continue;
		}

		FString FromNodeId, FromPinName;
		FString ToNodeId, ToPinName;

		if (!(*FromObjectPtr)->TryGetStringField("nodeId", FromNodeId) ||
			!(*FromObjectPtr)->TryGetStringField("pinName", FromPinName) ||
			!(*ToObjectPtr)->TryGetStringField("nodeId", ToNodeId) ||
			!(*ToObjectPtr)->TryGetStringField("pinName", ToPinName))
		{
			continue;
		}

		// Find nodes and pins
		UEdGraphNode* FromNode = FindNodeById(Graph, FromNodeId);
		UEdGraphNode* ToNode = FindNodeById(Graph, ToNodeId);

		if (!FromNode || !ToNode)
		{
			continue;
		}

		UEdGraphPin* FromPin = FindPinByName(FromNode, FromPinName);
		UEdGraphPin* ToPin = FindPinByName(ToNode, ToPinName);

		if (FromPin && ToPin)
		{
			// Create connection
			FromPin->MakeLinkTo(ToPin);
		}
	}

	return true;
}

bool FBlueprintGraphDeserializer::ValidateJsonSchema(const TSharedPtr<FJsonObject>& JsonData)
{
	if (!JsonData.IsValid())
	{
		return false;
	}

	// Use schema validator
	return FBlueprintGraphJsonSchema::ValidateJson(JsonData);
}

UEdGraphNode* FBlueprintGraphDeserializer::FindNodeById(UEdGraph* Graph, const FString& NodeId)
{
	if (!Graph || NodeId.IsEmpty())
	{
		return nullptr;
	}

	// Try to parse as GUID first
	FGuid NodeGuid;
	if (FGuid::Parse(NodeId, NodeGuid))
	{
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (Node && Node->NodeGuid == NodeGuid)
			{
				return Node;
			}
		}
	}

	// Fallback: search by string ID (for generated IDs)
	// This is a simplified implementation - in practice, we might need to store a mapping
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (Node)
		{
			// For now, we'll need to improve this when node creation is implemented
			// and we can properly track IDs
		}
	}

	return nullptr;
}

UEdGraphPin* FBlueprintGraphDeserializer::FindPinByName(UEdGraphNode* Node, const FString& PinName)
{
	if (!Node || PinName.IsEmpty())
	{
		return nullptr;
	}

	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin && Pin->PinName.ToString() == PinName)
		{
			return Pin;
		}
	}

	return nullptr;
}

