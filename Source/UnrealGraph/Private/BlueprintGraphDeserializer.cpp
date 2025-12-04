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
#include "UObject/UObjectGlobals.h"
#include "UObject/Class.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"
#include "UObject/PropertyAccessUtil.h"
#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/ConstructorHelpers.h"

// Static map to track node ID mappings during deserialization
static TMap<FString, UEdGraphNode*> GNodeIdMap;

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

	// Clear the node ID mapping for this deserialization
	GNodeIdMap.Empty();

	// Get graph data
	const TSharedPtr<FJsonObject>* GraphObjectPtr;
	if (!JsonData->TryGetObjectField(TEXT("graph"), GraphObjectPtr))
	{
		UE_LOG(LogTemp, Error, TEXT("Missing 'graph' field in JSON"));
		return false;
	}

	TSharedPtr<FJsonObject> GraphObject = *GraphObjectPtr;

	// Create nodes first
	const TArray<TSharedPtr<FJsonValue>>* NodesArray;
	if (GraphObject->TryGetArrayField(TEXT("nodes"), NodesArray))
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

	// Create connections after all nodes are created
	const TArray<TSharedPtr<FJsonValue>>* ConnectionsArray;
	if (GraphObject->TryGetArrayField(TEXT("connections"), ConnectionsArray))
	{
		UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Attempting to create %d connections"), ConnectionsArray->Num());
		int32 SuccessfulConnections = CreateConnectionsFromJson(Graph, *ConnectionsArray);
		UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Successfully created %d/%d connections"), SuccessfulConnections, ConnectionsArray->Num());
	}

	// Clear the mapping after connections are created
	GNodeIdMap.Empty();

	// Mark Blueprint as modified
	if (UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph))
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	}

	UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Deserialization completed. Created %d nodes"), 
		NodesArray ? NodesArray->Num() : 0);

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
	if (!NodeData->TryGetStringField(TEXT("type"), NodeType))
	{
		UE_LOG(LogTemp, Warning, TEXT("Missing 'type' field in node data"));
		return nullptr;
	}

	// Get node ID for mapping
	FString NodeId;
	if (!NodeData->TryGetStringField(TEXT("id"), NodeId))
	{
		UE_LOG(LogTemp, Warning, TEXT("Missing 'id' field in node data"));
		return nullptr;
	}

	// Map node type to UClass
	UClass* NodeClass = GetNodeClassFromTypeName(NodeType);
	if (!NodeClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not find UClass for node type: %s"), *NodeType);
		return nullptr;
	}

	// Create the node
	UEdGraphNode* NewNode = NewObject<UEdGraphNode>(Graph, NodeClass, NAME_None, RF_Transactional);
	if (!NewNode)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create node of type: %s"), *NodeType);
		return nullptr;
	}

	// Add node to graph first (some nodes need to be in graph before configuration)
	Graph->AddNode(NewNode, /*bFromUI*/ false, /*bSelectNewNode*/ false);

	// Configure node-specific properties BEFORE allocating pins
	bool bWasConfigured = ConfigureNodeProperties(NewNode, NodeData, Graph);
	
	// Reconstruct node if configuration changed it (some nodes need this to allocate pins properly)
	// ReconstructNode will allocate pins based on the configured properties
	if (bWasConfigured)
	{
		NewNode->ReconstructNode();
	}
	else
	{
		// If not configured or doesn't need configuration, just allocate default pins
		NewNode->AllocateDefaultPins();
	}

	// Restore pin default values from JSON
	RestorePinDefaultValues(NewNode, NodeData);

	// Set node position (after adding to graph)
	SetNodePosition(NewNode, NodeData);

	// Post-creation setup - some nodes need this
	NewNode->PostPlacedNewNode();

	// Store mapping of old ID to new node
	GNodeIdMap.Add(NodeId, NewNode);

	// Log node creation and available pins for debugging
	FString PinList;
	for (UEdGraphPin* Pin : NewNode->Pins)
	{
		if (Pin)
		{
			if (!PinList.IsEmpty()) PinList += TEXT(", ");
			PinList += FString::Printf(TEXT("%s(%s)"), *Pin->PinName.ToString(), Pin->Direction == EGPD_Input ? TEXT("in") : TEXT("out"));
		}
	}
	UE_LOG(LogTemp, Log, TEXT("Created node: %s (ID: %s) with %d pins: %s"), 
		*NodeType, *NodeId, NewNode->Pins.Num(), *PinList);

	return NewNode;
}

int32 FBlueprintGraphDeserializer::CreateConnectionsFromJson(UEdGraph* Graph, const TArray<TSharedPtr<FJsonValue>>& ConnectionsArray)
{
	int32 SuccessCount = 0;
	
	if (!Graph)
	{
		return 0;
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
		
		if (!ConnectionObject->TryGetObjectField(TEXT("from"), FromObjectPtr) ||
			!ConnectionObject->TryGetObjectField(TEXT("to"), ToObjectPtr))
		{
			continue;
		}

		FString FromNodeId, FromPinName;
		FString ToNodeId, ToPinName;

		if (!(*FromObjectPtr)->TryGetStringField(TEXT("nodeId"), FromNodeId) ||
			!(*FromObjectPtr)->TryGetStringField(TEXT("pinName"), FromPinName) ||
			!(*ToObjectPtr)->TryGetStringField(TEXT("nodeId"), ToNodeId) ||
			!(*ToObjectPtr)->TryGetStringField(TEXT("pinName"), ToPinName))
		{
			continue;
		}

		// Find nodes and pins
		UEdGraphNode* FromNode = FindNodeById(Graph, FromNodeId);
		UEdGraphNode* ToNode = FindNodeById(Graph, ToNodeId);

		if (!FromNode)
		{
			UE_LOG(LogTemp, Warning, TEXT("UnrealGraph: Could not find FromNode with ID: %s"), *FromNodeId);
			continue;
		}

		if (!ToNode)
		{
			UE_LOG(LogTemp, Warning, TEXT("UnrealGraph: Could not find ToNode with ID: %s"), *ToNodeId);
			continue;
		}

		UEdGraphPin* FromPin = FindPinByName(FromNode, FromPinName);
		UEdGraphPin* ToPin = FindPinByName(ToNode, ToPinName);

		if (!FromPin)
		{
			UE_LOG(LogTemp, Warning, TEXT("UnrealGraph: Could not find FromPin '%s' on node %s"), *FromPinName, *FromNode->GetName());
			// Log available pins for debugging
			FString AvailablePins;
			for (UEdGraphPin* Pin : FromNode->Pins)
			{
				if (Pin)
				{
					if (!AvailablePins.IsEmpty()) AvailablePins += TEXT(", ");
					AvailablePins += Pin->PinName.ToString();
				}
			}
			UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Available pins on FromNode: %s"), *AvailablePins);
			continue;
		}

		if (!ToPin)
		{
			UE_LOG(LogTemp, Warning, TEXT("UnrealGraph: Could not find ToPin '%s' on node %s"), *ToPinName, *ToNode->GetName());
			// Log available pins for debugging
			FString AvailablePins;
			for (UEdGraphPin* Pin : ToNode->Pins)
			{
				if (Pin)
				{
					if (!AvailablePins.IsEmpty()) AvailablePins += TEXT(", ");
					AvailablePins += Pin->PinName.ToString();
				}
			}
			UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Available pins on ToNode: %s"), *AvailablePins);
			continue;
		}

		// Create connection - MakeLinkTo returns void, so we can't check success directly
		// Check if pins are already connected before attempting
		bool bAlreadyConnected = FromPin->LinkedTo.Contains(ToPin);
		if (!bAlreadyConnected)
		{
			FromPin->MakeLinkTo(ToPin);
			SuccessCount++;
			UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Created connection from %s.%s to %s.%s"), 
				*FromNode->GetName(), *FromPinName, *ToNode->GetName(), *ToPinName);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Connection from %s.%s to %s.%s already exists"), 
				*FromNode->GetName(), *FromPinName, *ToNode->GetName(), *ToPinName);
			SuccessCount++; // Count as success since it's already connected
		}
	}

	return SuccessCount;
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

	// First, check the mapping from deserialization (for newly created nodes)
	if (UEdGraphNode** MappedNode = GNodeIdMap.Find(NodeId))
	{
		if (*MappedNode && IsValid(*MappedNode))
		{
			return *MappedNode;
		}
	}

	// Try to parse as GUID and search in existing nodes
	FGuid NodeGuid;
	if (FGuid::Parse(NodeId, NodeGuid))
	{
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (Node && IsValid(Node) && Node->NodeGuid == NodeGuid)
			{
				return Node;
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Could not find node with ID: %s"), *NodeId);
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

UClass* FBlueprintGraphDeserializer::GetNodeClassFromTypeName(const FString& NodeTypeName)
{
	if (NodeTypeName.IsEmpty())
	{
		return nullptr;
	}

	// Build the full class path - Blueprint graph nodes are typically in BlueprintGraph module
	// Format: /Script/ModuleName.ClassName
	FString ClassPath = FString::Printf(TEXT("/Script/BlueprintGraph.%s"), *NodeTypeName);
	
	// Try to load the class
	UClass* NodeClass = LoadClass<UEdGraphNode>(nullptr, *ClassPath);
	
	if (!NodeClass)
	{
		// Try iterating through all UEdGraphNode classes to find by name
		for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
		{
			UClass* TestClass = *ClassIt;
			if (TestClass && TestClass->IsChildOf(UEdGraphNode::StaticClass()))
			{
				if (TestClass->GetName() == NodeTypeName)
				{
					NodeClass = TestClass;
					break;
				}
			}
		}
	}

	return NodeClass;
}

void FBlueprintGraphDeserializer::SetNodePosition(UEdGraphNode* Node, const TSharedPtr<FJsonObject>& NodeData)
{
	if (!Node || !NodeData.IsValid())
	{
		return;
	}

	// Get position from JSON
	const TSharedPtr<FJsonObject>* PositionObjectPtr;
	if (!NodeData->TryGetObjectField(TEXT("position"), PositionObjectPtr))
	{
		return;
	}

	TSharedPtr<FJsonObject> PositionObject = *PositionObjectPtr;
	
	double X = 0.0, Y = 0.0;
	PositionObject->TryGetNumberField(TEXT("x"), X);
	PositionObject->TryGetNumberField(TEXT("y"), Y);

	FVector2D NewPosition(static_cast<float>(X), static_cast<float>(Y));

	// Try to set NodePos via reflection (same approach as serializer)
	FProperty* PosProperty = Node->GetClass()->FindPropertyByName(TEXT("NodePos"));
	if (!PosProperty)
	{
		PosProperty = FindFProperty<FProperty>(Node->GetClass(), TEXT("NodePos"));
	}

	if (PosProperty)
	{
		FStructProperty* StructProp = CastField<FStructProperty>(PosProperty);
		if (StructProp && StructProp->Struct && StructProp->Struct->GetFName() == NAME_Vector2D)
		{
			FVector2D* PosPtr = StructProp->ContainerPtrToValuePtr<FVector2D>(Node);
			if (PosPtr)
			{
				*PosPtr = NewPosition;
				Node->Modify(); // Mark for undo/redo
			}
		}
	}
	
	// Note: Some nodes may need additional setup or notification after position change
	// This basic implementation should work for most node types
}

bool FBlueprintGraphDeserializer::ConfigureNodeProperties(UEdGraphNode* Node, const TSharedPtr<FJsonObject>& NodeData, UEdGraph* Graph)
{
	if (!Node || !NodeData.IsValid() || !Graph)
	{
		return false;
	}

	FString NodeType;
	if (!NodeData->TryGetStringField(TEXT("type"), NodeType))
	{
		return false;
	}

	FString Title;
	NodeData->TryGetStringField(TEXT("title"), Title);

	// Get Blueprint for variable lookups
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
	if (!Blueprint)
	{
		return false;
	}

	// Configure based on node type
	if (NodeType == TEXT("K2Node_CallFunction"))
	{
		// Extract function name from title (e.g., "Print String" -> "PrintString")
		FString FunctionName = Title.Replace(TEXT(" "), TEXT(""));
		
		// Try to find the function - first check if we have explicit functionName in JSON
		FString ExplicitFunctionName;
		if (NodeData->TryGetStringField(TEXT("functionName"), ExplicitFunctionName))
		{
			FunctionName = ExplicitFunctionName;
		}

		// Common function mapping (title -> actual function name)
		if (Title == TEXT("Print String"))
		{
			FunctionName = TEXT("PrintString");
		}

		UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Configuring CallFunction node with function: %s"), *FunctionName);
		
		// Find the UFunction by name - try common classes
		UFunction* TargetFunction = nullptr;
		
		// Try UKismetSystemLibrary first (most common for PrintString, etc.)
		if (UClass* KismetLibClass = UKismetSystemLibrary::StaticClass())
		{
			TargetFunction = KismetLibClass->FindFunctionByName(*FunctionName);
		}
		
		// If not found, search in common classes
		if (!TargetFunction)
		{
			// Try to find in any class
			for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
			{
				UClass* TestClass = *ClassIt;
				if (TestClass && TestClass->IsChildOf(UObject::StaticClass()))
				{
					TargetFunction = TestClass->FindFunctionByName(*FunctionName);
					if (TargetFunction)
					{
						break;
					}
				}
			}
		}
		
		if (!TargetFunction)
		{
			UE_LOG(LogTemp, Warning, TEXT("UnrealGraph: Could not find function: %s"), *FunctionName);
			return false;
		}
		
		// Set the function reference using reflection
		// FunctionReference is typically a FMemberReference struct
		FStructProperty* FunctionRefProp = CastField<FStructProperty>(Node->GetClass()->FindPropertyByName(TEXT("FunctionReference")));
		if (FunctionRefProp && FunctionRefProp->Struct)
		{
			// Get pointer to FunctionReference struct
			void* FunctionRefPtr = FunctionRefProp->ContainerPtrToValuePtr<void>(Node);
			if (FunctionRefPtr)
			{
				// Try to set MemberName
				if (FNameProperty* MemberNameProp = CastField<FNameProperty>(FunctionRefProp->Struct->FindPropertyByName(TEXT("MemberName"))))
				{
					MemberNameProp->SetPropertyValue_InContainer(FunctionRefPtr, TargetFunction->GetFName());
				}
				
				// Try to set MemberParent (the class)
				if (FObjectProperty* MemberParentProp = CastField<FObjectProperty>(FunctionRefProp->Struct->FindPropertyByName(TEXT("MemberParent"))))
				{
					MemberParentProp->SetObjectPropertyValue_InContainer(FunctionRefPtr, TargetFunction->GetOuterUClass());
				}
				
				UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Set FunctionReference for %s"), *FunctionName);
				return true;
			}
		}
		
		UE_LOG(LogTemp, Warning, TEXT("UnrealGraph: Could not set FunctionReference property on node"));
		return false;
	}
	else if (NodeType == TEXT("K2Node_VariableGet"))
	{
		// Extract variable name from title (e.g., "Get In String" -> "In String")
		FString VariableName = Title;
		if (VariableName.StartsWith(TEXT("Get ")))
		{
			VariableName = VariableName.RightChop(4); // Remove "Get "
		}

		// Try explicit variableName in JSON
		FString ExplicitVariableName;
		if (NodeData->TryGetStringField(TEXT("variableName"), ExplicitVariableName))
		{
			VariableName = ExplicitVariableName;
		}

		UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Need to configure VariableGet node with variable: %s (from title: %s)"), *VariableName, *Title);
		
		// TODO: Find variable in Blueprint and set it on the node
		return true;
	}
	else if (NodeType == TEXT("K2Node_Event"))
	{
		// Extract event name from title (e.g., "Event BeginPlay" -> "BeginPlay")
		FString EventName = Title;
		if (EventName.StartsWith(TEXT("Event ")))
		{
			EventName = EventName.RightChop(6); // Remove "Event "
		}

		// Try explicit eventName in JSON
		FString ExplicitEventName;
		if (NodeData->TryGetStringField(TEXT("eventName"), ExplicitEventName))
		{
			EventName = ExplicitEventName;
		}

		UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Configuring Event node with event: %s"), *EventName);
		
		// Check if we have the event class path from serialization (most reliable)
		FString EventClassPath;
		FString EventClassName;
		if (NodeData->TryGetStringField(TEXT("eventClassPath"), EventClassPath))
		{
			// Load the class from the path
			UClass* EventClass = LoadClass<UObject>(nullptr, *EventClassPath);
			if (EventClass)
			{
				UFunction* EventFunction = EventClass->FindFunctionByName(*EventName);
				if (EventFunction)
				{
					// Set EventReference using the exact function we found
					FStructProperty* EventRefProp = CastField<FStructProperty>(Node->GetClass()->FindPropertyByName(TEXT("EventReference")));
					if (EventRefProp && EventRefProp->Struct)
					{
						void* EventRefPtr = EventRefProp->ContainerPtrToValuePtr<void>(Node);
						if (EventRefPtr)
						{
							// Clear CustomFunctionName first (standard events shouldn't have it set)
							FNameProperty* CustomFunctionNameProp = CastField<FNameProperty>(Node->GetClass()->FindPropertyByName(TEXT("CustomFunctionName")));
							if (CustomFunctionNameProp)
							{
								CustomFunctionNameProp->SetPropertyValue_InContainer(Node, NAME_None);
							}
							
							// Set MemberName (function name)
							if (FNameProperty* MemberNameProp = CastField<FNameProperty>(EventRefProp->Struct->FindPropertyByName(TEXT("MemberName"))))
							{
								MemberNameProp->SetPropertyValue_InContainer(EventRefPtr, EventFunction->GetFName());
							}
							
							// Set MemberParent (the class containing the function)
							if (FObjectProperty* MemberParentProp = CastField<FObjectProperty>(EventRefProp->Struct->FindPropertyByName(TEXT("MemberParent"))))
							{
								MemberParentProp->SetObjectPropertyValue_InContainer(EventRefPtr, EventClass);
							}
							
							UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Set EventReference for %s from serialized class path %s"), 
								*EventName, *EventClassPath);
							return true;
						}
					}
				}
			}
		}
		
		// Fallback: Try to find the function by searching common classes
		UFunction* EventFunction = nullptr;
		
		// Try common event locations - most events are in AActor
		if (UClass* ActorClass = AActor::StaticClass())
		{
			EventFunction = ActorClass->FindFunctionByName(*EventName);
		}
		
		// If not found in AActor, search in other common classes
		if (!EventFunction)
		{
			// Search in all classes
			for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
			{
				UClass* TestClass = *ClassIt;
				if (TestClass && TestClass->IsChildOf(UObject::StaticClass()))
				{
					EventFunction = TestClass->FindFunctionByName(*EventName);
					if (EventFunction)
					{
						break;
					}
				}
			}
		}
		
		// Set EventReference struct if we found the function
		FStructProperty* EventRefProp = CastField<FStructProperty>(Node->GetClass()->FindPropertyByName(TEXT("EventReference")));
		if (EventRefProp && EventRefProp->Struct && EventFunction)
		{
			void* EventRefPtr = EventRefProp->ContainerPtrToValuePtr<void>(Node);
			if (EventRefPtr)
			{
				// Clear CustomFunctionName first (standard events shouldn't have it set)
				FNameProperty* CustomFunctionNameProp = CastField<FNameProperty>(Node->GetClass()->FindPropertyByName(TEXT("CustomFunctionName")));
				if (CustomFunctionNameProp)
				{
					CustomFunctionNameProp->SetPropertyValue_InContainer(Node, NAME_None);
				}
				
				// Set MemberName (function name)
				if (FNameProperty* MemberNameProp = CastField<FNameProperty>(EventRefProp->Struct->FindPropertyByName(TEXT("MemberName"))))
				{
					MemberNameProp->SetPropertyValue_InContainer(EventRefPtr, EventFunction->GetFName());
				}
				
				// Set MemberParent (the class containing the function)
				if (FObjectProperty* MemberParentProp = CastField<FObjectProperty>(EventRefProp->Struct->FindPropertyByName(TEXT("MemberParent"))))
				{
					UClass* FunctionClass = EventFunction->GetOuterUClass();
					MemberParentProp->SetObjectPropertyValue_InContainer(EventRefPtr, FunctionClass);
				}
				
				UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Set EventReference for %s from class %s"), 
					*EventName, *EventFunction->GetOuterUClass()->GetName());
				return true;
			}
		}
		
		// Last resort: Set as custom event if we can't find the standard event
		bool bIsCustomEvent = false;
		NodeData->TryGetBoolField(TEXT("isCustomEvent"), bIsCustomEvent);
		
		if (bIsCustomEvent)
		{
			FNameProperty* CustomFunctionNameProp = CastField<FNameProperty>(Node->GetClass()->FindPropertyByName(TEXT("CustomFunctionName")));
			if (CustomFunctionNameProp)
			{
				CustomFunctionNameProp->SetPropertyValue_InContainer(Node, *EventName);
				UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Set CustomFunctionName to %s (custom event)"), *EventName);
			}
		}
		
		return true;
	}

	return false;
}

void FBlueprintGraphDeserializer::RestorePinDefaultValues(UEdGraphNode* Node, const TSharedPtr<FJsonObject>& NodeData)
{
	if (!Node || !NodeData.IsValid())
	{
		return;
	}

	// Get pins array from JSON
	const TArray<TSharedPtr<FJsonValue>>* PinsArrayPtr;
	if (!NodeData->TryGetArrayField(TEXT("pins"), PinsArrayPtr))
	{
		return;
	}

	// Create a map of pin names to default values from JSON
	TMap<FString, FString> PinDefaultValues;
	for (const TSharedPtr<FJsonValue>& PinValue : *PinsArrayPtr)
	{
		const TSharedPtr<FJsonObject>* PinObjectPtr;
		if (PinValue->TryGetObject(PinObjectPtr))
		{
			FString PinName;
			FString DefaultValue;
			
			if ((*PinObjectPtr)->TryGetStringField(TEXT("name"), PinName) &&
				(*PinObjectPtr)->TryGetStringField(TEXT("defaultValue"), DefaultValue))
			{
				PinDefaultValues.Add(PinName, DefaultValue);
			}
		}
	}

	// Restore default values on node pins
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin && PinDefaultValues.Contains(Pin->PinName.ToString()))
		{
			FString DefaultValue = PinDefaultValues[Pin->PinName.ToString()];
			Pin->DefaultValue = DefaultValue;
			Pin->AutogeneratedDefaultValue = DefaultValue;
		}
	}
}

