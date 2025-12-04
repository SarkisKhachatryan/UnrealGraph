// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class UEdGraph;
class UEdGraphNode;

/**
 * Deserializes JSON format back to Blueprint graphs
 */
class FBlueprintGraphDeserializer
{
public:
	/**
	 * Deserialize JSON data into a Blueprint graph
	 * @param Graph The target graph to populate
	 * @param JsonData The JSON object containing graph data
	 * @return True if deserialization succeeded, false otherwise
	 */
	static bool DeserializeGraph(UEdGraph* Graph, const TSharedPtr<FJsonObject>& JsonData);

	/**
	 * Create a node from JSON data
	 * @param Graph The graph to add the node to
	 * @param NodeData The JSON object containing node data
	 * @return The created node, or nullptr if creation failed
	 */
	static UEdGraphNode* CreateNodeFromJson(UEdGraph* Graph, const TSharedPtr<FJsonObject>& NodeData);

	/**
	 * Create connections from JSON data
	 * @param Graph The graph containing the nodes
	 * @param ConnectionsArray The JSON array containing connection data
	 * @return Number of successfully created connections
	 */
	static int32 CreateConnectionsFromJson(UEdGraph* Graph, const TArray<TSharedPtr<FJsonValue>>& ConnectionsArray);

	/**
	 * Validate JSON schema
	 * @param JsonData The JSON object to validate
	 * @return True if JSON structure is valid
	 */
	static bool ValidateJsonSchema(const TSharedPtr<FJsonObject>& JsonData);

private:
	/**
	 * Find a node by its ID
	 * @param Graph The graph to search in
	 * @param NodeId The ID of the node to find
	 * @return The found node, or nullptr if not found
	 */
	static UEdGraphNode* FindNodeById(UEdGraph* Graph, const FString& NodeId);

	/**
	 * Find a pin by name on a node
	 * @param Node The node to search
	 * @param PinName The name of the pin
	 * @return The found pin, or nullptr if not found
	 */
	static class UEdGraphPin* FindPinByName(UEdGraphNode* Node, const FString& PinName);

	/**
	 * Map JSON node type name to UClass
	 * @param NodeTypeName The class name from JSON (e.g., "K2Node_Event")
	 * @return The UClass for that node type, or nullptr if not found
	 */
	static UClass* GetNodeClassFromTypeName(const FString& NodeTypeName);

	/**
	 * Set node position from JSON data
	 * @param Node The node to set position for
	 * @param NodeData The JSON object containing position data
	 */
	static void SetNodePosition(UEdGraphNode* Node, const TSharedPtr<FJsonObject>& NodeData);

	/**
	 * Configure node-specific properties before allocating pins
	 * @param Node The node to configure
	 * @param NodeData The JSON object containing node data
	 * @param Graph The graph the node belongs to
	 * @return True if configuration succeeded
	 */
	static bool ConfigureNodeProperties(UEdGraphNode* Node, const TSharedPtr<FJsonObject>& NodeData, UEdGraph* Graph);

	/**
	 * Restore pin default values from JSON
	 * @param Node The node to restore pin values for
	 * @param NodeData The JSON object containing pin data
	 */
	static void RestorePinDefaultValues(UEdGraphNode* Node, const TSharedPtr<FJsonObject>& NodeData);
};

