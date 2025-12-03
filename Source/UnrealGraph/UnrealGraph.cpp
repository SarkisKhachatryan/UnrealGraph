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
		return nullptr;
	}
	
	// Get all open Blueprint editors
	// Note: GetBlueprintEditors() returns TArray<TSharedRef<IBlueprintEditor>>
	// TSharedRef is always valid (unlike TSharedPtr)
	auto OpenEditors = BlueprintEditorModule->GetBlueprintEditors();
	
	// Try all open editors - prefer ones with focused graphs
	UEdGraph* BestGraph = nullptr;
	
	for (const auto& Editor : OpenEditors)
	{
		// TSharedRef is always valid, so we can use it directly
		// Try to get the focused graph from this editor
		UEdGraph* FocusedGraph = Editor->GetFocusedGraph();
		if (FocusedGraph && IsValid(FocusedGraph))
		{
			// Get the Blueprint from the graph using BlueprintEditorUtils
			UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(FocusedGraph);
			if (Blueprint)
			{
				BestGraph = FocusedGraph;
				UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Found focused graph from Blueprint editor: %s - %s (%d nodes)"), 
					*Blueprint->GetName(), *BestGraph->GetName(), BestGraph->Nodes.Num());
				return BestGraph;
			}
		}
	}
	
	// No focused graph - try to find the most recently accessed editor
	// Use the last editor in the list (often the most recent)
	if (OpenEditors.Num() > 0)
	{
		const auto& LastEditor = OpenEditors[OpenEditors.Num() - 1];
		
		// Try to get any graph from this editor
		UEdGraph* FocusedGraph = LastEditor->GetFocusedGraph();
		if (FocusedGraph && IsValid(FocusedGraph))
		{
			UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(FocusedGraph);
			if (Blueprint)
			{
				UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Using focused graph from most recent Blueprint editor: %s - %s (%d nodes)"), 
					*Blueprint->GetName(), *FocusedGraph->GetName(), FocusedGraph->Nodes.Num());
				return FocusedGraph;
			}
		}
		
		// Fallback: try to get the Blueprint from the editor and use its event graph
		// We'll need to find another way to get the Blueprint - for now, use a simpler approach
		// Get the graph from the focused graph's owning Blueprint
		UEdGraph* AnyFocusedGraph = LastEditor->GetFocusedGraph();
		if (AnyFocusedGraph)
		{
			UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(AnyFocusedGraph);
			if (Blueprint && Blueprint->UbergraphPages.Num() > 0)
			{
				UEdGraph* EventGraph = Blueprint->UbergraphPages[0];
				if (EventGraph && IsValid(EventGraph))
				{
					UE_LOG(LogTemp, Log, TEXT("UnrealGraph: Using event graph from most recent Blueprint: %s - %s (%d nodes)"), 
						*Blueprint->GetName(), *EventGraph->GetName(), EventGraph->Nodes.Num());
					return EventGraph;
				}
			}
		}
	}
	
	return nullptr;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUnrealGraphModule, UnrealGraph)

