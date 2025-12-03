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
	 * @return True if connections were created successfully
	 */
	static bool CreateConnectionsFromJson(UEdGraph* Graph, const TArray<TSharedPtr<FJsonValue>>& ConnectionsArray);

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
};

