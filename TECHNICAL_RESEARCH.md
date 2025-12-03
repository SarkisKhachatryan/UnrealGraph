# Technical Research - Unreal Graph

This document contains research notes and findings about Unreal Engine's Blueprint system that are needed for implementing the Unreal Graph plugin.

## Blueprint Graph System Architecture

### Core Classes

#### UEdGraph
- **Purpose**: Container for graph nodes and connections
- **Location**: `Engine/Source/Editor/GraphEditor/Public/EdGraph.h`
- **Key Properties**:
  - `TArray<UEdGraphNode*> Nodes` - All nodes in the graph
  - `TArray<UEdGraph*> SubGraphs` - Nested graphs (functions, macros)
- **Key Methods**:
  - `UEdGraphNode* CreateNode()` - Create a new node
  - `void RemoveNode()` - Remove a node

#### UEdGraphNode
- **Purpose**: Base class for all graph nodes
- **Location**: `Engine/Source/Editor/GraphEditor/Public/EdGraphNode.h`
- **Key Properties**:
  - `FVector2D NodePos` - Node position in graph
  - `FText NodeComment` - Node comment/description
  - `TArray<UEdGraphPin*> Pins` - Input/output pins
  - `FString NodeTitle` - Display title
- **Key Methods**:
  - `void AllocateDefaultPins()` - Create default pins
  - `void PinConnectionListChanged()` - Called when connections change

#### UK2Node
- **Purpose**: Base class for Blueprint-specific nodes
- **Location**: `Engine/Source/Editor/BlueprintGraph/Public/K2Node.h`
- **Subclasses**:
  - `UK2Node_Event` - Event nodes (BeginPlay, Tick, etc.)
  - `UK2Node_CallFunction` - Function call nodes
  - `UK2Node_Variable` - Variable get/set nodes
  - `UK2Node_IfThenElse` - Branch nodes
  - `UK2Node_ExecutionSequence` - Sequence nodes

#### UEdGraphPin
- **Purpose**: Represents a connection point on a node
- **Location**: `Engine/Source/Editor/GraphEditor/Public/EdGraphPin.h`
- **Key Properties**:
  - `FString PinName` - Pin name
  - `EEdGraphPinDirection Direction` - Input or Output
  - `FEdGraphPinType PinType` - Data type
  - `TArray<UEdGraphPin*> LinkedTo` - Connected pins
  - `FString DefaultValue` - Default value (as string)

#### UBlueprint
- **Purpose**: Blueprint asset representation
- **Location**: `Engine/Source/Engine/Classes/Blueprint/Blueprint.h`
- **Key Properties**:
  - `UEdGraph* SimpleConstructionScript` - Construction graph
  - `UEdGraph* EventGraph` - Event graph
  - `TArray<UEdGraph*> FunctionGraphs` - Function graphs
  - `TArray<UEdGraph*> MacroGraphs` - Macro graphs

### Graph Traversal

To serialize a Blueprint graph, we need to:

1. **Get the graph**: Access `UBlueprint::EventGraph` or specific function graph
2. **Iterate nodes**: Loop through `UEdGraph::Nodes`
3. **Extract node data**:
   - Node type/class
   - Position
   - Properties
   - Pins
4. **Extract connections**: For each pin, get `LinkedTo` array
5. **Handle nested graphs**: Recursively process `SubGraphs`

### Node Creation

To deserialize and create nodes:

1. **Parse JSON**: Extract node data from JSON
2. **Determine node class**: Map JSON node type to `UEdGraphNode` subclass
3. **Create node**: Use `UEdGraph::CreateNode()` or `FGraphNodeCreator`
4. **Set properties**: Use reflection system to set node properties
5. **Create pins**: Call `AllocateDefaultPins()` or manually create pins
6. **Set position**: Set `NodePos` property
7. **Create connections**: Use `UEdGraphPin::MakeLinkTo()`

### Key APIs

#### FGraphNodeCreator
- **Purpose**: Helper class for creating nodes
- **Usage**:
```cpp
FGraphNodeCreator<NodeClass> NodeCreator(*Graph);
NodeClass* NewNode = NodeCreator.CreateNode();
// Set properties
NodeCreator.Finalize();
```

#### FBlueprintEditorUtils
- **Purpose**: Utility functions for Blueprint editing
- **Location**: `Engine/Source/Editor/BlueprintGraph/Public/BlueprintEditorUtils.h`
- **Key Methods**:
  - `AddNewNode()` - Add node to graph
  - `RemoveNode()` - Remove node from graph
  - `MarkBlueprintAsModified()` - Mark Blueprint as dirty

#### FEdGraphSchemaAction
- **Purpose**: Represents an action that can be performed in the graph
- **Usage**: Used for context menu actions

## JSON Serialization in Unreal Engine

### FJsonObject and FJsonValue

Unreal Engine provides JSON support through:
- `TSharedPtr<FJsonObject>` - JSON object
- `TSharedPtr<FJsonValue>` - JSON value
- `FJsonSerializer` - Serialization utilities

**Location**: `Engine/Source/Runtime/Json/Public/Json.h`

### Serialization Example

```cpp
// Create JSON object
TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
JsonObject->SetStringField("name", "MyNode");
JsonObject->SetNumberField("x", 100.0);

// Serialize to string
FString OutputString;
TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
```

### Deserialization Example

```cpp
// Parse JSON string
TSharedPtr<FJsonObject> JsonObject;
TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
FJsonSerializer::Deserialize(Reader, JsonObject);

// Read values
FString Name = JsonObject->GetStringField("name");
double X = JsonObject->GetNumberField("x");
```

## Editor Integration

### Module Setup

Plugin module structure:
```cpp
class FUnrealGraphEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
```

### Context Menu Extension

To add items to Blueprint editor context menu:

```cpp
// In StartupModule()
FBlueprintEditorModule& BlueprintEditorModule = 
    FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
    
TSharedPtr<FExtensibilityManager> MenuExtensibilityManager = 
    BlueprintEditorModule.GetMenuExtensibilityManager();
    
MenuExtensibilityManager->AddExtender(MenuExtender);
```

### Keyboard Shortcuts

Register commands:
```cpp
FUICommandList& CommandList = BlueprintEditor->GetToolkitCommands();
CommandList.MapAction(
    FUnrealGraphCommands::Get().CopyAsJSON,
    FExecuteAction::CreateRaw(this, &FUnrealGraphEditorModule::OnCopyAsJSON)
);
```

### Clipboard Access

```cpp
// Copy to clipboard
FPlatformApplicationMisc::ClipboardCopy(*JsonString);

// Paste from clipboard
FString ClipboardContent;
FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);
```

## Node Type Handling

### Common Node Types

1. **Event Nodes** (`UK2Node_Event`)
   - BeginPlay, Tick, Custom Events
   - Properties: Event name, Event signature

2. **Function Nodes** (`UK2Node_CallFunction`)
   - Function calls
   - Properties: Function reference, Target object

3. **Variable Nodes** (`UK2Node_Variable`)
   - Get/Set variable
   - Properties: Variable reference

4. **Flow Control** (`UK2Node_IfThenElse`, `UK2Node_ExecutionSequence`)
   - Branches, sequences
   - Properties: Condition (for branches)

5. **Literals** (`UK2Node_Literal`)
   - Constant values
   - Properties: Value, Type

### Node Property Serialization

Use Unreal's reflection system:
```cpp
// Get property value
FProperty* Property = NodeClass->FindPropertyByName("PropertyName");
void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Node);
FString ValueString = Property->ExportTextItem(ValuePtr, ...);

// Set property value
Property->ImportText(*ValueString, ValuePtr, ...);
```

## Challenges & Solutions

### Challenge 1: Node Type Identification
**Problem**: Need to identify node class from JSON
**Solution**: Store full class path in JSON, use `FindObject<UClass>()` or `LoadClass()`

### Challenge 2: Pin Type Serialization
**Problem**: Complex pin types (structs, objects) need special handling
**Solution**: Serialize `FEdGraphPinType` structure, handle defaults separately

### Challenge 3: Asset References
**Problem**: References to other assets (textures, sounds, etc.)
**Solution**: Store asset path, use `FAssetData` or `FSoftObjectPath`

### Challenge 4: Undo/Redo Support
**Problem**: Graph modifications should support undo
**Solution**: Use `FScopedTransaction` for all graph modifications

### Challenge 5: Transaction Management
**Problem**: Multiple operations need to be atomic
**Solution**: Wrap operations in transactions

## Testing Approach

### Unit Testing
- Test individual node serialization
- Test connection serialization
- Test JSON schema validation

### Integration Testing
- Test full serialize/deserialize cycle
- Test with various Blueprint types
- Test with complex graphs

### Manual Testing
- Test with real-world Blueprints
- Test edge cases (empty graphs, disconnected nodes, etc.)

## Research Questions

1. **How are custom nodes handled?**
   - Need to investigate custom node serialization
   - May require plugin-specific handling

2. **How to handle compiled Blueprints?**
   - Can we access graph data from compiled Blueprints?
   - Or do we need source Blueprints?

3. **Performance considerations?**
   - How large can graphs be?
   - What's the serialization overhead?

4. **Version compatibility?**
   - How do node types change between UE versions?
   - Do we need version-specific handling?

## Next Research Steps

1. **Create test plugin** to explore Blueprint API
2. **Study existing plugins** (BPtoJSON, GraphPrinter) for reference
3. **Experiment with node creation** to understand the process
4. **Test JSON serialization** with simple graphs
5. **Investigate clipboard integration** in Unreal Editor

## References

- [Unreal Engine Source Code](https://github.com/EpicGames/UnrealEngine)
- [Blueprint API Documentation](https://docs.unrealengine.com/5.3/en-US/blueprint-api-in-unreal-engine/)
- [Plugin Development Guide](https://docs.unrealengine.com/5.3/en-US/plugins-in-unreal-engine/)
- [Editor Extensions](https://docs.unrealengine.com/5.3/en-US/extending-the-editor-in-unreal-engine/)

