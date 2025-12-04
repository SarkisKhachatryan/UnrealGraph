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
#include "UObject/PropertyAccessUtil.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

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

	// Position - Get node position
	// Note: Node positions in UE5 are stored but may need to be accessed via graph editor
	// For now, we'll serialize as (0,0) but log that positions need to be fixed
	// TODO: Access node positions through graph editor view or layout system
	TSharedPtr<FJsonObject> PositionObject = MakeShareable(new FJsonObject);
	FVector2D NodePosition(0.0f, 0.0f);
	
	// Try accessing NodePos through multiple methods
	// Method 1: Direct property access via FindPropertyByName
	if (FProperty* PosProp = Node->GetClass()->FindPropertyByName(TEXT("NodePos")))
	{
		if (FStructProperty* StructProp = CastField<FStructProperty>(PosProp))
		{
			if (StructProp->Struct && StructProp->Struct->GetFName() == NAME_Vector2D)
			{
				const FVector2D* PosPtr = StructProp->ContainerPtrToValuePtr<FVector2D>(Node);
				if (PosPtr)
				{
					NodePosition = *PosPtr;
				}
			}
		}
	}
	
	// Method 2: Try through base class if not found
	if (NodePosition.X == 0.0f && NodePosition.Y == 0.0f)
	{
		UClass* BaseClass = Node->GetClass()->GetSuperClass();
		if (BaseClass)
		{
			if (FProperty* PosProp = BaseClass->FindPropertyByName(TEXT("NodePos")))
			{
				if (FStructProperty* StructProp = CastField<FStructProperty>(PosProp))
				{
					if (StructProp->Struct && StructProp->Struct->GetFName() == NAME_Vector2D)
					{
						const FVector2D* PosPtr = StructProp->ContainerPtrToValuePtr<FVector2D>(Node);
						if (PosPtr)
						{
							NodePosition = *PosPtr;
						}
					}
				}
			}
		}
	}
	
	// Note: If position is still (0,0), we'll need to access it through graph editor view system
	// This is a known limitation that will be addressed in a future update
	PositionObject->SetNumberField(TEXT("x"), static_cast<double>(NodePosition.X));
	PositionObject->SetNumberField(TEXT("y"), static_cast<double>(NodePosition.Y));
	NodeObject->SetObjectField(TEXT("position"), PositionObject);

	// Comment - Serialize node comment
	// TODO: Fix NodeComment serialization - FText conversion needs proper API
	// Comment serialization temporarily disabled due to compilation issues
	// Will be fixed in a future update once the correct FText API is determined

	// Serialize node-specific properties (function refs, variable refs, etc.)
	SerializeNodeProperties(Node, NodeObject);

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

void FBlueprintGraphSerializer::SerializeNodeProperties(UEdGraphNode* Node, TSharedPtr<FJsonObject>& NodeObject)
{
	if (!Node || !NodeObject.IsValid())
	{
		return;
	}

	FString NodeTypeName = GetNodeClassName(Node);

	// Serialize properties for K2Node_CallFunction
	if (NodeTypeName == TEXT("K2Node_CallFunction"))
	{
		// Try to get function reference using reflection
		if (FObjectProperty* FunctionProp = CastField<FObjectProperty>(Node->GetClass()->FindPropertyByName(TEXT("FunctionReference"))))
		{
			// FunctionReference is a struct, need to access it differently
		}
		else if (FStructProperty* FunctionRefProp = CastField<FStructProperty>(Node->GetClass()->FindPropertyByName(TEXT("FunctionReference"))))
		{
			// Access the function reference struct and extract member name
			void* FunctionRefPtr = FunctionRefProp->ContainerPtrToValuePtr<void>(Node);
			if (FunctionRefPtr)
			{
				// Try to get MemberName from the struct
				if (FNameProperty* MemberNameProp = CastField<FNameProperty>(FunctionRefProp->Struct->FindPropertyByName(TEXT("MemberName"))))
				{
					FName MemberName = MemberNameProp->GetPropertyValue_InContainer(FunctionRefPtr);
					if (!MemberName.IsNone())
					{
						NodeObject->SetStringField(TEXT("functionName"), MemberName.ToString());
					}
				}
			}
		}
	}

	// Serialize properties for K2Node_VariableGet
	else if (NodeTypeName == TEXT("K2Node_VariableGet"))
	{
		// Try to get variable reference
		if (FStructProperty* VariableRefProp = CastField<FStructProperty>(Node->GetClass()->FindPropertyByName(TEXT("VariableReference"))))
		{
			void* VariableRefPtr = VariableRefProp->ContainerPtrToValuePtr<void>(Node);
			if (VariableRefPtr)
			{
				if (FNameProperty* MemberNameProp = CastField<FNameProperty>(VariableRefProp->Struct->FindPropertyByName(TEXT("MemberName"))))
				{
					FName MemberName = MemberNameProp->GetPropertyValue_InContainer(VariableRefPtr);
					if (!MemberName.IsNone())
					{
						NodeObject->SetStringField(TEXT("variableName"), MemberName.ToString());
					}
				}
			}
		}
	}

	// Serialize properties for K2Node_Event
	else if (NodeTypeName == TEXT("K2Node_Event"))
	{
		// Try EventReference first (for standard events like BeginPlay)
		FStructProperty* EventRefProp = CastField<FStructProperty>(Node->GetClass()->FindPropertyByName(TEXT("EventReference")));
		if (EventRefProp && EventRefProp->Struct)
		{
			void* EventRefPtr = EventRefProp->ContainerPtrToValuePtr<void>(Node);
			if (EventRefPtr)
			{
				// Get MemberName (event function name)
				if (FNameProperty* MemberNameProp = CastField<FNameProperty>(EventRefProp->Struct->FindPropertyByName(TEXT("MemberName"))))
				{
					FName MemberName = MemberNameProp->GetPropertyValue_InContainer(EventRefPtr);
					if (!MemberName.IsNone())
					{
						NodeObject->SetStringField(TEXT("eventName"), MemberName.ToString());
						
						// Get MemberParent (the class containing the event) - important for standard events
						if (FObjectProperty* MemberParentProp = CastField<FObjectProperty>(EventRefProp->Struct->FindPropertyByName(TEXT("MemberParent"))))
						{
							UObject* MemberParent = MemberParentProp->GetObjectPropertyValue_InContainer(EventRefPtr);
							if (MemberParent)
							{
								UClass* ParentClass = Cast<UClass>(MemberParent);
								if (ParentClass)
								{
									NodeObject->SetStringField(TEXT("eventClass"), ParentClass->GetName());
									NodeObject->SetStringField(TEXT("eventClassPath"), ParentClass->GetPathName());
								}
							}
						}
					}
				}
			}
		}
		
		// Fallback: Try CustomFunctionName (for custom events)
		if (!NodeObject->HasField(TEXT("eventName")))
		{
			if (FNameProperty* EventNameProp = CastField<FNameProperty>(Node->GetClass()->FindPropertyByName(TEXT("CustomFunctionName"))))
			{
				FName EventName = EventNameProp->GetPropertyValue_InContainer(Node);
				if (!EventName.IsNone())
				{
					NodeObject->SetStringField(TEXT("eventName"), EventName.ToString());
					NodeObject->SetBoolField(TEXT("isCustomEvent"), true);
				}
			}
		}
	}
}

FString FBlueprintGraphSerializer::JsonToString(const TSharedPtr<FJsonObject>& JsonObject, bool bPrettyPrint)
{
	if (!JsonObject.IsValid())
	{
		return FString();
	}

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	// If pretty printing is needed, we can format manually or use a different writer
	// For now, return the basic serialization
	return OutputString;
}

