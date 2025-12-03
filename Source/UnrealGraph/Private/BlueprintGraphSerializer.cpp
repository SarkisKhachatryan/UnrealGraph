// Copyright Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphSerializer.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Internationalization/Text.h"
#include "HAL/Platform.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"

TSharedPtr<FJsonObject> FBlueprintGraphSerializer::SerializeGraph(UEdGraph* Graph)
{
	if (!Graph)
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);

	// Add metadata
	TSharedPtr<FJsonObject> MetadataObject = MakeShareable(new FJsonObject);
	MetadataObject->SetStringField(TEXT("version"), TEXT("1.0"));
	MetadataObject->SetStringField(TEXT("unrealVersion"), TEXT("5.3.0"));
	MetadataObject->SetStringField(TEXT("exportDate"), FDateTime::Now().ToIso8601());
	RootObject->SetObjectField(TEXT("metadata"), MetadataObject);

	// Serialize nodes
	TArray<TSharedPtr<FJsonValue>> NodesArray;
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (Node)
		{
			TSharedPtr<FJsonObject> NodeObject = SerializeNode(Node);
			if (NodeObject.IsValid())
			{
				NodesArray.Add(MakeShareable(new FJsonValueObject(NodeObject)));
			}
		}
	}

	// Serialize connections
	TArray<TSharedPtr<FJsonValue>> ConnectionsArray = SerializeConnections(Graph);

	// Build graph object
	TSharedPtr<FJsonObject> GraphObject = MakeShareable(new FJsonObject);
	GraphObject->SetArrayField(TEXT("nodes"), NodesArray);
	GraphObject->SetArrayField(TEXT("connections"), ConnectionsArray);

	RootObject->SetObjectField(TEXT("graph"), GraphObject);

	return RootObject;
}

TSharedPtr<FJsonObject> FBlueprintGraphSerializer::SerializeNode(UEdGraphNode* Node)
{
	if (!Node)
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> NodeObject = MakeShareable(new FJsonObject);

	// Basic node information
	NodeObject->SetStringField(TEXT("id"), GetNodeId(Node));
	NodeObject->SetStringField(TEXT("type"), GetNodeClassName(Node));
	NodeObject->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());

	// Position - TODO: Fix NodePos access (temporarily set to 0,0)
	// The NodePos property access method varies by UE version
	TSharedPtr<FJsonObject> PositionObject = MakeShareable(new FJsonObject);
	FVector2D NodePosition(0.0f, 0.0f);
	
	// Try to get position - will implement properly once we know UE version
	// For now, default to (0,0) to allow compilation
	PositionObject->SetNumberField(TEXT("x"), static_cast<double>(NodePosition.X));
	PositionObject->SetNumberField(TEXT("y"), static_cast<double>(NodePosition.Y));
	NodeObject->SetObjectField(TEXT("position"), PositionObject);

	// Comment - Temporarily disabled to fix compilation error
	// TODO: Fix NodeComment serialization - type resolution issue needs investigation
	// The error suggests NodeComment type may differ by UE version

	// Serialize pins
	TArray<TSharedPtr<FJsonValue>> PinsArray;
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin)
		{
			TSharedPtr<FJsonObject> PinObject = SerializePin(Pin);
			if (PinObject.IsValid())
			{
				PinsArray.Add(MakeShareable(new FJsonValueObject(PinObject)));
			}
		}
	}
	NodeObject->SetArrayField(TEXT("pins"), PinsArray);

	return NodeObject;
}

TSharedPtr<FJsonObject> FBlueprintGraphSerializer::SerializePin(UEdGraphPin* Pin)
{
	if (!Pin)
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> PinObject = MakeShareable(new FJsonObject);

	PinObject->SetStringField(TEXT("name"), Pin->PinName.ToString());
	PinObject->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("input") : TEXT("output"));
	
	// Pin type information
	PinObject->SetStringField(TEXT("pinCategory"), Pin->PinType.PinCategory.ToString());
	if (!Pin->PinType.PinSubCategory.IsNone())
	{
		PinObject->SetStringField(TEXT("pinSubCategory"), Pin->PinType.PinSubCategory.ToString());
	}

	// Default value
	if (!Pin->DefaultValue.IsEmpty())
	{
		PinObject->SetStringField(TEXT("defaultValue"), Pin->DefaultValue);
	}

	// Store connected node IDs (will be used for connections array)
	TArray<TSharedPtr<FJsonValue>> ConnectedNodeIds;
	for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
	{
		if (LinkedPin && LinkedPin->GetOwningNode())
		{
			ConnectedNodeIds.Add(MakeShareable(new FJsonValueString(GetNodeId(LinkedPin->GetOwningNode()))));
		}
	}
	if (ConnectedNodeIds.Num() > 0)
	{
		PinObject->SetArrayField(TEXT("connectedNodeIds"), ConnectedNodeIds);
	}

	return PinObject;
}

TArray<TSharedPtr<FJsonValue>> FBlueprintGraphSerializer::SerializeConnections(UEdGraph* Graph)
{
	TArray<TSharedPtr<FJsonValue>> ConnectionsArray;

	if (!Graph)
	{
		return ConnectionsArray;
	}

	// Iterate through all nodes and their pins to find connections
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (!Node)
		{
			continue;
		}

		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (!Pin || Pin->Direction != EGPD_Output)
			{
				continue;
			}

			// Create connection entries for each linked pin
			for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
			{
				if (LinkedPin && LinkedPin->GetOwningNode())
				{
					TSharedPtr<FJsonObject> ConnectionObject = MakeShareable(new FJsonObject);

					// From pin
					TSharedPtr<FJsonObject> FromObject = MakeShareable(new FJsonObject);
					FromObject->SetStringField(TEXT("nodeId"), GetNodeId(Node));
					FromObject->SetStringField(TEXT("pinName"), Pin->PinName.ToString());
					ConnectionObject->SetObjectField(TEXT("from"), FromObject);

					// To pin
					TSharedPtr<FJsonObject> ToObject = MakeShareable(new FJsonObject);
					ToObject->SetStringField(TEXT("nodeId"), GetNodeId(LinkedPin->GetOwningNode()));
					ToObject->SetStringField(TEXT("pinName"), LinkedPin->PinName.ToString());
					ConnectionObject->SetObjectField(TEXT("to"), ToObject);

					ConnectionsArray.Add(MakeShareable(new FJsonValueObject(ConnectionObject)));
				}
			}
		}
	}

	return ConnectionsArray;
}

FString FBlueprintGraphSerializer::GetNodeId(UEdGraphNode* Node)
{
	if (!Node)
	{
		return FString();
	}

	// Use node GUID if available, otherwise generate from pointer
	FGuid NodeGuid = Node->NodeGuid;
	if (NodeGuid.IsValid())
	{
		return NodeGuid.ToString();
	}

	// Fallback: use a combination of class name and pointer address
	return FString::Printf(TEXT("node_%s_%p"), *Node->GetClass()->GetName(), Node);
}

FString FBlueprintGraphSerializer::GetNodeClassName(UEdGraphNode* Node)
{
	if (!Node)
	{
		return FString();
	}

	return Node->GetClass()->GetName();
}

