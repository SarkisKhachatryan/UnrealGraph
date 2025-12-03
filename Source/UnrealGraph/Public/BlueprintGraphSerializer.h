// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"

/**
 * Serializes Blueprint graphs to JSON format
 */
class FBlueprintGraphSerializer
{
public:
	/**
	 * Serialize an entire Blueprint graph to JSON
	 * @param Graph The graph to serialize
	 * @return Shared pointer to JSON object containing graph data
	 */
	static TSharedPtr<FJsonObject> SerializeGraph(UEdGraph* Graph);

	/**
	 * Serialize a single node to JSON
	 * @param Node The node to serialize
	 * @return Shared pointer to JSON object containing node data
	 */
	static TSharedPtr<FJsonObject> SerializeNode(UEdGraphNode* Node);

	/**
	 * Serialize a pin to JSON
	 * @param Pin The pin to serialize
	 * @return Shared pointer to JSON object containing pin data
	 */
	static TSharedPtr<FJsonObject> SerializePin(UEdGraphPin* Pin);

	/**
	 * Serialize all connections in a graph
	 * @param Graph The graph containing connections
	 * @return Array of JSON values representing connections
	 */
	static TArray<TSharedPtr<FJsonValue>> SerializeConnections(UEdGraph* Graph);

private:
	/**
	 * Generate a unique ID for a node
	 * @param Node The node to generate an ID for
	 * @return Unique identifier string
	 */
	static FString GetNodeId(UEdGraphNode* Node);

	/**
	 * Get the node class name for serialization
	 * @param Node The node to get the class name for
	 * @return Class name string
	 */
	static FString GetNodeClassName(UEdGraphNode* Node);
};

