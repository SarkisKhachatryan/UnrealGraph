// Copyright Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphSerializer.h"
#include "UnrealGraphLogger.h"
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

	// Initialize logger for this serialization session
	FUnrealGraphLogger::Initialize(TEXT("UnrealGraph_Serialization"));
	FUnrealGraphLogger::LogSection(FString::Printf(TEXT("Serializing Graph: %s"), *Graph->GetName()));
	FUnrealGraphLogger::LogFormatted(TEXT("Graph has %d nodes"), Graph->Nodes.Num());

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

	// Log completion and shutdown logger
	FUnrealGraphLogger::LogSection(TEXT("Serialization Complete"));
	FUnrealGraphLogger::LogFormatted(TEXT("Successfully serialized %d nodes and %d connections"), NodesArray.Num(), ConnectionsArray.Num());
	FUnrealGraphLogger::Shutdown();

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

	// Log node details for analysis
	FUnrealGraphLogger::LogNodeDetails(Node);

	// Position - Get node position
	FUnrealGraphLogger::LogSection(FString::Printf(TEXT("Position Detection for Node: %s"), *Node->GetName()));
	TSharedPtr<FJsonObject> PositionObject = MakeShareable(new FJsonObject);
	FVector2D NodePosition(0.0f, 0.0f);
	bool bPositionFound = false;
	
	// Method 1: Try multiple property name variations
	FUnrealGraphLogger::Log(TEXT("Method 1: Searching for Vector2D position properties..."));
	const TArray<FName> PositionPropertyNames = {
		TEXT("NodePos"),
		TEXT("NodePosition"),
		TEXT("Position"),
		TEXT("Pos")
	};
	
	for (const FName& PropName : PositionPropertyNames)
	{
		FUnrealGraphLogger::LogFormatted(TEXT("  Checking property: %s"), *PropName.ToString());
		
		// Try in current class
		if (FProperty* PosProp = Node->GetClass()->FindPropertyByName(PropName))
		{
			FUnrealGraphLogger::LogFormatted(TEXT("    Found property '%s' in class '%s'"), *PropName.ToString(), *Node->GetClass()->GetName());
			
			if (FStructProperty* StructProp = CastField<FStructProperty>(PosProp))
			{
				if (StructProp->Struct && StructProp->Struct->GetFName() == NAME_Vector2D)
				{
					const FVector2D* PosPtr = StructProp->ContainerPtrToValuePtr<FVector2D>(Node);
					if (PosPtr)
					{
						NodePosition = *PosPtr;
						bPositionFound = true;
						FUnrealGraphLogger::LogFormatted(TEXT("    ✓ SUCCESS: Found position via property %s: (%.1f, %.1f)"), 
							*PropName.ToString(), NodePosition.X, NodePosition.Y);
						UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Found position via property %s: (%.1f, %.1f)"), 
							*PropName.ToString(), NodePosition.X, NodePosition.Y);
						break;
					}
					else
					{
						FUnrealGraphLogger::Log(TEXT("    ✗ Property found but pointer is null"));
					}
				}
				else
				{
					FUnrealGraphLogger::LogFormatted(TEXT("    ✗ Property found but not Vector2D type"));
				}
			}
		}
		else
		{
			FUnrealGraphLogger::Log(TEXT("    ✗ Property not found in current class"));
		}
		
		// Try through all base classes
		if (!bPositionFound)
		{
			FUnrealGraphLogger::Log(TEXT("    Checking base classes..."));
			for (UClass* Class = Node->GetClass(); Class && !bPositionFound; Class = Class->GetSuperClass())
			{
				if (FProperty* PosProp = Class->FindPropertyByName(PropName))
				{
					FUnrealGraphLogger::LogFormatted(TEXT("      Found property '%s' in base class '%s'"), *PropName.ToString(), *Class->GetName());
					
					if (FStructProperty* StructProp = CastField<FStructProperty>(PosProp))
					{
						if (StructProp->Struct && StructProp->Struct->GetFName() == NAME_Vector2D)
						{
							const FVector2D* PosPtr = StructProp->ContainerPtrToValuePtr<FVector2D>(Node);
							if (PosPtr)
							{
								NodePosition = *PosPtr;
								bPositionFound = true;
								FUnrealGraphLogger::LogFormatted(TEXT("      ✓ SUCCESS: Found position via property %s in class %s: (%.1f, %.1f)"), 
									*PropName.ToString(), *Class->GetName(), NodePosition.X, NodePosition.Y);
								UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Found position via property %s in class %s: (%.1f, %.1f)"), 
									*PropName.ToString(), *Class->GetName(), NodePosition.X, NodePosition.Y);
								break;
							}
						}
					}
				}
			}
		}
		
		if (bPositionFound)
		{
			break;
		}
	}
	
	// Method 2: Try separate X and Y properties (NodePosX, NodePosY)
	if (!bPositionFound)
	{
		FUnrealGraphLogger::Log(TEXT("Method 2: Searching for separate X/Y position properties..."));
		float PosX = 0.0f;
		float PosY = 0.0f;
		bool bHasX = false;
		bool bHasY = false;
		
		// Try to find NodePosX - NOTE: It's an IntProperty, not FloatProperty!
		for (const FString& XName : { TEXT("NodePosX"), TEXT("PosX"), TEXT("PositionX") })
		{
			// Try IntProperty first (this is what Unreal actually uses!)
			if (FIntProperty* XProp = FindFProperty<FIntProperty>(Node->GetClass(), *XName))
			{
				int32 IntValue = XProp->GetPropertyValue_InContainer(Node);
				PosX = static_cast<float>(IntValue);
				bHasX = true;
				FUnrealGraphLogger::LogFormatted(TEXT("  Found X position via IntProperty %s: %d (%.1f)"), *XName, IntValue, PosX);
				UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Found X position via IntProperty %s: %d"), *XName, IntValue);
				break;
			}
			// Fallback to FloatProperty (in case some nodes use it)
			else if (FFloatProperty* XPropFloat = FindFProperty<FFloatProperty>(Node->GetClass(), *XName))
			{
				PosX = XPropFloat->GetPropertyValue_InContainer(Node);
				bHasX = true;
				FUnrealGraphLogger::LogFormatted(TEXT("  Found X position via FloatProperty %s: %.1f"), *XName, PosX);
				break;
			}
		}
		
		// Try to find NodePosY - NOTE: It's an IntProperty, not FloatProperty!
		for (const FString& YName : { TEXT("NodePosY"), TEXT("PosY"), TEXT("PositionY") })
		{
			// Try IntProperty first (this is what Unreal actually uses!)
			if (FIntProperty* YProp = FindFProperty<FIntProperty>(Node->GetClass(), *YName))
			{
				int32 IntValue = YProp->GetPropertyValue_InContainer(Node);
				PosY = static_cast<float>(IntValue);
				bHasY = true;
				FUnrealGraphLogger::LogFormatted(TEXT("  Found Y position via IntProperty %s: %d (%.1f)"), *YName, IntValue, PosY);
				UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Found Y position via IntProperty %s: %d"), *YName, IntValue);
				break;
			}
			// Fallback to FloatProperty (in case some nodes use it)
			else if (FFloatProperty* YPropFloat = FindFProperty<FFloatProperty>(Node->GetClass(), *YName))
			{
				PosY = YPropFloat->GetPropertyValue_InContainer(Node);
				bHasY = true;
				FUnrealGraphLogger::LogFormatted(TEXT("  Found Y position via FloatProperty %s: %.1f"), *YName, PosY);
				break;
			}
		}
		
		if (bHasX && bHasY)
		{
			NodePosition = FVector2D(PosX, PosY);
			bPositionFound = true;
			UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Found position via separate X/Y properties: (%.1f, %.1f)"), 
				NodePosition.X, NodePosition.Y);
		}
	}
	
	// Method 3: Iterate through all properties to find any Vector2D
	if (!bPositionFound)
	{
		FUnrealGraphLogger::Log(TEXT("Method 3: Iterating through all properties to find Vector2D..."));
		int32 Vector2DCount = 0;
		
		for (TFieldIterator<FProperty> PropIt(Node->GetClass()); PropIt && !bPositionFound; ++PropIt)
		{
			FProperty* Prop = *PropIt;
			if (FStructProperty* StructProp = CastField<FStructProperty>(Prop))
			{
				if (StructProp->Struct && StructProp->Struct->GetFName() == NAME_Vector2D)
				{
					Vector2DCount++;
					FName PropName = Prop->GetFName();
					FString PropNameStr = PropName.ToString();
					
					FUnrealGraphLogger::LogFormatted(TEXT("  Found Vector2D property: %s"), *PropNameStr);
					
					if (PropNameStr.Contains(TEXT("Pos")) || PropNameStr.Contains(TEXT("Position")))
					{
						const FVector2D* PosPtr = StructProp->ContainerPtrToValuePtr<FVector2D>(Node);
						if (PosPtr)
						{
							FUnrealGraphLogger::LogFormatted(TEXT("    Property value: (%.1f, %.1f)"), PosPtr->X, PosPtr->Y);
							
							if (PosPtr->X != 0.0f || PosPtr->Y != 0.0f)
							{
								NodePosition = *PosPtr;
								bPositionFound = true;
								FUnrealGraphLogger::LogFormatted(TEXT("    ✓ SUCCESS: Found position via property iteration: %s = (%.1f, %.1f)"), 
									*PropNameStr, NodePosition.X, NodePosition.Y);
								UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Found position via property iteration: %s = (%.1f, %.1f)"), 
									*PropNameStr, NodePosition.X, NodePosition.Y);
								break;
							}
							else
							{
								FUnrealGraphLogger::Log(TEXT("    ✗ Position is (0,0) - skipping"));
							}
						}
					}
				}
			}
		}
		
		FUnrealGraphLogger::LogFormatted(TEXT("  Total Vector2D properties found: %d"), Vector2DCount);
	}
	
	// Serialize position (even if (0,0) - deserialization can still use it)
	PositionObject->SetNumberField(TEXT("x"), static_cast<double>(NodePosition.X));
	PositionObject->SetNumberField(TEXT("y"), static_cast<double>(NodePosition.Y));
	NodeObject->SetObjectField(TEXT("position"), PositionObject);
	
	if (!bPositionFound && NodePosition.X == 0.0f && NodePosition.Y == 0.0f)
	{
		FUnrealGraphLogger::Log(TEXT("✗ FAILED: Node position not found, serializing as (0,0)"));
		UE_LOG(LogTemp, Warning, TEXT("UnrealGraph: Node position not found for %s, serializing as (0,0)"), *Node->GetName());
	}
	else if (bPositionFound)
	{
		FUnrealGraphLogger::LogFormatted(TEXT("✓ SUCCESS: Final position: (%.1f, %.1f)"), NodePosition.X, NodePosition.Y);
		UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Serializing node %s position: (%.1f, %.1f)"), 
			*Node->GetName(), NodePosition.X, NodePosition.Y);
	}

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

	// Serialize properties for K2Node_VariableGet and K2Node_VariableSet
	else if (NodeTypeName == TEXT("K2Node_VariableGet") || NodeTypeName == TEXT("K2Node_VariableSet"))
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

