// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnrealGraph.h"
#include "UnrealGraphCommands.h"
#include "UnrealGraphStyle.h"
#include "BlueprintGraphSerializer.h"
#include "BlueprintGraphDeserializer.h"
#include "ToolMenus.h"
#include "HAL/IConsoleManager.h"
#include "BlueprintEditorModule.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ToolMenuSection.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EdGraph/EdGraph.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Framework/Application/SlateApplication.h"
#include "Toolkits/AssetEditorToolkit.h"

#define LOCTEXT_NAMESPACE "FUnrealGraphModule"

void FUnrealGraphModule::StartupModule()
{
	// Initialize style
	FUnrealGraphStyle::Initialize();
	
	// Register commands
	FUnrealGraphCommands::Register();
	
	// Register menu extensions
	RegisterMenus();
	
	// Register console commands for testing
	RegisterConsoleCommands();
	
	// Create command list and bind actions
	CommandList = MakeShareable(new FUICommandList);
	CommandList->MapAction(
		FUnrealGraphCommands::Get().CopyAsJSON,
		FExecuteAction::CreateRaw(this, &FUnrealGraphModule::OnCopyAsJSON),
		FCanExecuteAction()
	);
	CommandList->MapAction(
		FUnrealGraphCommands::Get().PasteFromJSON,
		FExecuteAction::CreateRaw(this, &FUnrealGraphModule::OnPasteFromJSON),
		FCanExecuteAction()
	);
	
	// Register Blueprint editor menu extensions using ToolMenus
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FUnrealGraphModule::RegisterBlueprintEditorMenus));
	
	UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Plugin loaded successfully!"));
}

void FUnrealGraphModule::ShutdownModule()
{
	// Unregister menu extensions
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	
	// Unregister commands
	FUnrealGraphCommands::Unregister();
	
	// Shutdown style
	FUnrealGraphStyle::Shutdown();
}

void FUnrealGraphModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);
}

void FUnrealGraphModule::RegisterConsoleCommands()
{
	// Console command to test serialization
	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("UnrealGraph.TestSerialize"),
		TEXT("Test serializing a Blueprint graph to JSON"),
		FConsoleCommandDelegate::CreateRaw(this, &FUnrealGraphModule::TestSerialization),
		ECVF_Default
	);
	
	// Console command to test deserialization from file
	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("UnrealGraph.TestDeserialize"),
		TEXT("Test deserializing a Blueprint graph from UnrealGraph_Test.json file"),
		FConsoleCommandDelegate::CreateRaw(this, &FUnrealGraphModule::TestDeserialization),
		ECVF_Default
	);
}

void FUnrealGraphModule::TestSerialization()
{
	UE_LOG(LogTemp, Warning, TEXT("UnrealGraph: TestSerialization called"));
	
	// Get the focused Blueprint graph and serialize it
	UEdGraph* Graph = GetFocusedBlueprintGraph();
	if (Graph)
	{
		UE_LOG(LogTemp, Warning, TEXT("UnrealGraph: Found graph: %s with %d nodes"), *Graph->GetName(), Graph->Nodes.Num());
		
		if (Graph->Nodes.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("UnrealGraph: Graph is empty (no nodes). Try opening a Blueprint with nodes and run the command again."));
			return;
		}
		
		if (Graph->Nodes.Num() < 2)
		{
			UE_LOG(LogTemp, Warning, TEXT("UnrealGraph: Graph has only 1 node (likely a function entry). Looking for graphs with multiple nodes..."));
		}
		
		TSharedPtr<FJsonObject> JsonData = FBlueprintGraphSerializer::SerializeGraph(Graph);
		if (JsonData.IsValid())
		{
			FString JsonString = FBlueprintGraphSerializer::JsonToString(JsonData, true);
			
			// Log first 1000 characters to see more of the structure
			FString ShortJson = JsonString.Len() > 1000 ? JsonString.Left(1000) + TEXT("...") : JsonString;
			UE_LOG(LogTemp, Warning, TEXT("UnrealGraph: Serialized graph to JSON (%d chars, %d nodes):\n%s"), 
				JsonString.Len(), Graph->Nodes.Num(), *ShortJson);
			
			// Also log full JSON to a file for inspection
			FString FilePath = FPaths::ProjectLogDir() / TEXT("UnrealGraph_Test.json");
			FFileHelper::SaveStringToFile(JsonString, *FilePath);
			UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Full JSON saved to: %s"), *FilePath);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("UnrealGraph: Failed to serialize graph"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UnrealGraph: No Blueprint graph found. Please open a Blueprint with nodes first."));
	}
}

void FUnrealGraphModule::TestDeserialization()
{
	UE_LOG(LogTemp, Warning, TEXT("UnrealGraph: TestDeserialization called"));
	
	// Get the focused graph
	UEdGraph* Graph = GetFocusedBlueprintGraph();
	if (!Graph)
	{
		UE_LOG(LogTemp, Warning, TEXT("UnrealGraph: No Blueprint graph is currently focused. Please open a Blueprint first."));
		return;
	}
	
	// Load JSON from file
	FString FilePath = FPaths::ProjectLogDir() / TEXT("UnrealGraph_Test.json");
	FString JsonContent;
	
	if (!FFileHelper::LoadFileToString(JsonContent, *FilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("UnrealGraph: Failed to load JSON file: %s"), *FilePath);
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Loaded JSON from file (%d characters)"), JsonContent.Len());
	
	// Parse JSON
	TSharedPtr<FJsonObject> JsonData;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
	
	if (!FJsonSerializer::Deserialize(Reader, JsonData) || !JsonData.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("UnrealGraph: Failed to parse JSON from file"));
		return;
	}
	
	// Deserialize
	if (FBlueprintGraphDeserializer::DeserializeGraph(Graph, JsonData))
	{
		UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Graph deserialized from file successfully!"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UnrealGraph: Failed to deserialize graph from file"));
	}
}

void FUnrealGraphModule::RegisterBlueprintEditorMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);
	
	// Extend the graph editor context menu
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("GraphEditor.GraphNodeContextMenu");
	
	FToolMenuSection& Section = Menu->AddSection("UnrealGraph", LOCTEXT("UnrealGraphSection", "Unreal Graph"));
	
	Section.AddMenuEntryWithCommandList(FUnrealGraphCommands::Get().CopyAsJSON, CommandList);
	Section.AddMenuEntryWithCommandList(FUnrealGraphCommands::Get().PasteFromJSON, CommandList);
}

void FUnrealGraphModule::OnCopyAsJSON()
{
	UEdGraph* Graph = GetFocusedBlueprintGraph();
	if (!Graph)
	{
		UE_LOG(LogTemp, Warning, TEXT("UnrealGraph: No Blueprint graph is currently focused"));
		return;
	}

	// Serialize the graph
	TSharedPtr<FJsonObject> JsonData = FBlueprintGraphSerializer::SerializeGraph(Graph);
	if (!JsonData.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("UnrealGraph: Failed to serialize graph"));
		return;
	}

	// Convert to string
	FString JsonString = FBlueprintGraphSerializer::JsonToString(JsonData, true);
	
	// Copy to clipboard
	FPlatformApplicationMisc::ClipboardCopy(*JsonString);
	
	UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Graph copied as JSON to clipboard"));
	UE_LOG(LogTemp, Log, TEXT("UnrealGraph: JSON length: %d characters"), JsonString.Len());
}

void FUnrealGraphModule::OnPasteFromJSON()
{
	// Get clipboard content
	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);
	
	if (ClipboardContent.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("UnrealGraph: Clipboard is empty"));
		return;
	}

	// Parse JSON
	TSharedPtr<FJsonObject> JsonData;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ClipboardContent);
	
	if (!FJsonSerializer::Deserialize(Reader, JsonData) || !JsonData.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("UnrealGraph: Failed to parse JSON from clipboard"));
		return;
	}

	// Get the focused graph
	UEdGraph* Graph = GetFocusedBlueprintGraph();
	if (!Graph)
	{
		UE_LOG(LogTemp, Warning, TEXT("UnrealGraph: No Blueprint graph is currently focused"));
		return;
	}

	// Deserialize
	if (FBlueprintGraphDeserializer::DeserializeGraph(Graph, JsonData))
	{
		UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Graph pasted from JSON successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UnrealGraph: Failed to paste graph from JSON"));
	}
}

UEdGraph* FUnrealGraphModule::GetFocusedBlueprintGraph() const
{
	// Try to get the graph from the currently active Blueprint editor
	FBlueprintEditorModule* BlueprintEditorModule = FModuleManager::GetModulePtr<FBlueprintEditorModule>("Kismet");
	if (!BlueprintEditorModule)
	{
		UE_LOG(LogTemp, Warning, TEXT("UnrealGraph: BlueprintEditorModule not found"));
		return nullptr;
	}
	
	// Get all open Blueprint editors
	auto OpenEditors = BlueprintEditorModule->GetBlueprintEditors();
	
	if (OpenEditors.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("UnrealGraph: No Blueprint editors are open"));
		return nullptr;
	}
	
	// Strategy: Find the editor that has keyboard/window focus
	// Check which widget has focus and trace back to the editor
	TSharedPtr<SWidget> FocusedWidget = FSlateApplication::Get().GetKeyboardFocusedWidget();
	
	// Try to find which editor contains the focused widget
	// First, check editors that have focused graphs (most likely to be active)
	UEdGraph* ActiveFocusedGraph = nullptr;
	UBlueprint* ActiveBlueprint = nullptr;
	
	for (const auto& Editor : OpenEditors)
	{
		UEdGraph* FocusedGraph = Editor->GetFocusedGraph();
		if (FocusedGraph && IsValid(FocusedGraph))
		{
			UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(FocusedGraph);
			if (Blueprint)
			{
				// This editor has a focused graph - prioritize it
				// If we haven't found one yet, or if this one seems more relevant, use it
				if (!ActiveFocusedGraph)
				{
					ActiveFocusedGraph = FocusedGraph;
					ActiveBlueprint = Blueprint;
				}
			}
		}
	}
	
	// If we found a focused graph, return it
	if (ActiveFocusedGraph && ActiveBlueprint)
	{
		UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Found active focused graph: %s - %s (%d nodes)"), 
			*ActiveBlueprint->GetName(), *ActiveFocusedGraph->GetName(), ActiveFocusedGraph->Nodes.Num());
		return ActiveFocusedGraph;
	}
	
	// Fallback: Use the last editor in the list (often the most recently accessed)
	// Reverse iteration to check most recent first
	for (int32 i = OpenEditors.Num() - 1; i >= 0; --i)
	{
		const auto& Editor = OpenEditors[i];
		UEdGraph* FocusedGraph = Editor->GetFocusedGraph();
		
		if (FocusedGraph && IsValid(FocusedGraph))
		{
			UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(FocusedGraph);
			if (Blueprint)
			{
				UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Using graph from recent editor: %s - %s (%d nodes)"), 
					*Blueprint->GetName(), *FocusedGraph->GetName(), FocusedGraph->Nodes.Num());
				return FocusedGraph;
			}
		}
	}
	
	// Last resort: Get any graph from the most recent editor's Blueprint
	if (OpenEditors.Num() > 0)
	{
		const auto& LastEditor = OpenEditors[OpenEditors.Num() - 1];
		UEdGraph* AnyGraph = LastEditor->GetFocusedGraph();
		
		if (AnyGraph)
		{
			UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(AnyGraph);
			if (Blueprint)
			{
				// Try to get the event graph if available
				if (Blueprint->UbergraphPages.Num() > 0)
				{
					UEdGraph* EventGraph = Blueprint->UbergraphPages[0];
					if (EventGraph && IsValid(EventGraph))
					{
						UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Using event graph as fallback: %s - %s (%d nodes)"), 
							*Blueprint->GetName(), *EventGraph->GetName(), EventGraph->Nodes.Num());
						return EventGraph;
					}
				}
			}
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("UnrealGraph: Could not find any valid graph from open editors"));
	return nullptr;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUnrealGraphModule, UnrealGraph)

