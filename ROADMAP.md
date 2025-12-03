# Unreal Graph - Project Roadmap

## Project Overview

**Unreal Graph** is a tool that enables seamless copying and pasting of Blueprint graphs between Unreal Editor instances using JSON as an intermediate format. This allows developers to:
- Copy a Blueprint graph from Unreal Editor and paste it as JSON
- Store Blueprint graphs in JSON format for version control, sharing, or external processing
- Paste JSON back into Unreal Editor to reconstruct the Blueprint graph

## Technical Requirements

### Prerequisites
- **Unreal Engine**: UE 5.0+ (recommended: UE 5.3+)
- **Development Environment**: 
  - Visual Studio 2022 (Windows)
  - Unreal Engine source code access (for plugin development)
- **Programming Languages**: C++ (primary), Blueprint (optional UI)
- **Knowledge Areas**:
  - Unreal Engine plugin development
  - Blueprint graph system internals
  - JSON serialization/deserialization
  - Unreal Engine editor extensions

### Key Unreal Engine APIs to Understand
- `UEdGraph` - The graph container
- `UEdGraphNode` - Base class for all graph nodes
- `UK2Node` - Blueprint-specific node types
- `FEdGraphPin` - Graph pin connections
- `UBlueprint` - Blueprint asset representation
- `FBlueprintEditor` - Blueprint editor interface
- `FJsonObject` / `FJsonValue` - Unreal's JSON handling

## Architecture Design

### Core Components

1. **Graph Serializer** (`FBlueprintGraphSerializer`)
   - Traverses Blueprint graph structure
   - Extracts node data, properties, and connections
   - Converts to JSON format

2. **Graph Deserializer** (`FBlueprintGraphDeserializer`)
   - Parses JSON structure
   - Creates nodes in Blueprint editor
   - Establishes connections
   - Restores layout and metadata

3. **Editor Integration** (`FUnrealGraphEditorModule`)
   - Context menu extensions
   - Keyboard shortcuts
   - Editor utility widget (optional)

4. **JSON Schema Handler** (`FBlueprintGraphJsonSchema`)
   - Validates JSON structure
   - Version management
   - Schema migration

### Data Flow

```
Blueprint Graph → Serializer → JSON → Clipboard/File
                                 ↓
Blueprint Graph ← Deserializer ← JSON ← Clipboard/File
```

## JSON Schema Design

### Proposed JSON Structure

```json
{
  "version": "1.0",
  "metadata": {
    "unrealVersion": "5.3.0",
    "exportDate": "2024-01-01T00:00:00Z",
    "blueprintType": "Actor"
  },
  "graph": {
    "nodes": [
      {
        "id": "node_001",
        "type": "UK2Node_Event",
        "class": "UK2Node_EventBeginPlay",
        "title": "Event BeginPlay",
        "position": {
          "x": 0,
          "y": 0
        },
        "pins": [
          {
            "name": "OutputDelegate",
            "direction": "output",
            "type": "exec",
            "connections": ["node_002"]
          }
        ],
        "properties": {
          "CustomFunctionName": "BeginPlay"
        }
      }
    ],
    "connections": [
      {
        "from": {
          "nodeId": "node_001",
          "pinName": "OutputDelegate"
        },
        "to": {
          "nodeId": "node_002",
          "pinName": "then"
        }
      }
    ],
    "comments": [
      {
        "id": "comment_001",
        "text": "Main Logic",
        "position": {"x": 100, "y": 100},
        "size": {"width": 200, "height": 150},
        "nodes": ["node_001", "node_002"]
      }
    ],
    "variables": [
      {
        "name": "MyVariable",
        "type": "int32",
        "defaultValue": "0"
      }
    ]
  }
}
```

## Development Phases

### Phase 1: Foundation & Research (Week 1-2)
**Goal**: Establish project structure and understand Blueprint internals

**Tasks**:
- [ ] Set up Unreal Engine plugin project structure
- [ ] Research Unreal Engine Blueprint graph API
- [ ] Create basic plugin module that loads in editor
- [ ] Study existing Blueprint graph traversal methods
- [ ] Document Blueprint node types and their properties
- [ ] Create test Blueprint with various node types

**Deliverables**:
- Working plugin that loads in Unreal Editor
- Documentation of Blueprint graph structure
- Test Blueprint assets

### Phase 2: Serialization (Week 3-4)
**Goal**: Implement graph-to-JSON conversion

**Tasks**:
- [ ] Implement graph traversal algorithm
- [ ] Extract node data (type, position, properties)
- [ ] Extract pin information and connections
- [ ] Handle different node types (Events, Functions, Variables, etc.)
- [ ] Serialize to JSON using Unreal's JSON API
- [ ] Handle edge cases (loops, disconnected nodes, etc.)
- [ ] Add metadata (version, timestamp, etc.)

**Deliverables**:
- `FBlueprintGraphSerializer` class
- JSON export functionality
- Unit tests for serialization

### Phase 3: Deserialization (Week 5-6)
**Goal**: Implement JSON-to-graph conversion

**Tasks**:
- [ ] Parse JSON structure
- [ ] Validate JSON schema
- [ ] Create nodes in Blueprint editor
- [ ] Set node properties and positions
- [ ] Establish pin connections
- [ ] Handle node type mapping
- [ ] Restore comments and layout
- [ ] Error handling for invalid JSON

**Deliverables**:
- `FBlueprintGraphDeserializer` class
- JSON import functionality
- Unit tests for deserialization

### Phase 4: Editor Integration (Week 7-8)
**Goal**: Add user-friendly UI for copy/paste operations

**Tasks**:
- [ ] Add context menu items to Blueprint editor
- [ ] Implement "Copy as JSON" functionality
- [ ] Implement "Paste from JSON" functionality
- [ ] Add keyboard shortcuts (Ctrl+Shift+C, Ctrl+Shift+V)
- [ ] Create editor utility widget (optional)
- [ ] Add progress indicators for large graphs
- [ ] Implement clipboard integration

**Deliverables**:
- Context menu extensions
- Keyboard shortcuts
- Clipboard support

### Phase 5: Advanced Features (Week 9-10)
**Goal**: Handle complex scenarios and edge cases

**Tasks**:
- [ ] Support for custom nodes
- [ ] Handle variable references
- [ ] Support for macros
- [ ] Handle nested graphs (functions, events)
- [ ] Preserve node comments and annotations
- [ ] Support for multiple selected nodes
- [ ] Add validation and error reporting
- [ ] Handle version migration

**Deliverables**:
- Extended node type support
- Macro handling
- Improved error messages

### Phase 6: Testing & Polish (Week 11-12)
**Goal**: Ensure reliability and user experience

**Tasks**:
- [ ] Comprehensive testing with various Blueprint types
- [ ] Performance optimization for large graphs
- [ ] Memory leak detection
- [ ] User documentation
- [ ] Example Blueprints
- [ ] Bug fixes
- [ ] Code cleanup and refactoring

**Deliverables**:
- Tested plugin
- Documentation
- Example files

## Implementation Details

### Plugin Structure

```
UnrealGraph/
├── Source/
│   └── UnrealGraph/
│       ├── UnrealGraph.Build.cs
│       ├── UnrealGraph.cpp
│       ├── UnrealGraph.h
│       ├── Public/
│       │   ├── UnrealGraphEditorModule.h
│       │   ├── BlueprintGraphSerializer.h
│       │   ├── BlueprintGraphDeserializer.h
│       │   └── BlueprintGraphJsonSchema.h
│       └── Private/
│           ├── UnrealGraphEditorModule.cpp
│           ├── BlueprintGraphSerializer.cpp
│           ├── BlueprintGraphDeserializer.cpp
│           └── BlueprintGraphJsonSchema.cpp
├── Resources/
│   └── Icon128.png
└── UnrealGraph.uplugin
```

### Key Classes

#### FBlueprintGraphSerializer
```cpp
class FBlueprintGraphSerializer
{
public:
    // Serialize entire graph
    TSharedPtr<FJsonObject> SerializeGraph(UEdGraph* Graph);
    
    // Serialize single node
    TSharedPtr<FJsonObject> SerializeNode(UEdGraphNode* Node);
    
    // Serialize connections
    TArray<TSharedPtr<FJsonValue>> SerializeConnections(UEdGraph* Graph);
};
```

#### FBlueprintGraphDeserializer
```cpp
class FBlueprintGraphDeserializer
{
public:
    // Deserialize JSON to graph
    bool DeserializeGraph(UEdGraph* Graph, const TSharedPtr<FJsonObject>& JsonData);
    
    // Create node from JSON
    UEdGraphNode* CreateNodeFromJson(UEdGraph* Graph, const TSharedPtr<FJsonObject>& NodeData);
    
    // Create connections from JSON
    bool CreateConnectionsFromJson(UEdGraph* Graph, const TArray<TSharedPtr<FJsonValue>>& Connections);
};
```

### Editor Integration Points

1. **Context Menu Extension**
   - Hook into `FBlueprintEditor::ExtendMenu()` 
   - Add "Copy Graph as JSON" and "Paste Graph from JSON" options

2. **Keyboard Shortcuts**
   - Register commands in `FBlueprintEditor::RegisterToolbarCommands()`
   - Map to Ctrl+Shift+C and Ctrl+Shift+V

3. **Clipboard Integration**
   - Use `FPlatformApplicationMisc::ClipboardCopy()` and `ClipboardPaste()`

## Challenges & Considerations

### Technical Challenges

1. **Node Type Mapping**
   - Different node classes need different serialization approaches
   - Custom nodes may require special handling
   - Solution: Use factory pattern for node serialization

2. **Pin Type Serialization**
   - Complex types (structs, objects) need special handling
   - Default values must be preserved
   - Solution: Use Unreal's property system for serialization

3. **Graph State Management**
   - Undo/redo support
   - Transaction management
   - Solution: Use Unreal's transaction system

4. **Performance**
   - Large graphs may be slow to serialize/deserialize
   - Solution: Implement async operations and progress indicators

5. **Version Compatibility**
   - Different UE versions may have different node types
   - Solution: Version schema and migration system

### Limitations

- **Custom Nodes**: May require manual serialization support
- **Asset References**: External asset references may need special handling
- **Compiled Blueprints**: Can only work with editable Blueprint graphs
- **Complex Types**: Some complex data types may not serialize perfectly

## Testing Strategy

### Unit Tests
- Test serialization of individual node types
- Test connection serialization
- Test JSON schema validation

### Integration Tests
- Test full graph serialization/deserialization cycle
- Test with various Blueprint types (Actor, Function, Animation)
- Test with complex graphs (loops, branches, custom nodes)

### User Testing
- Test with real-world Blueprint graphs
- Gather feedback on usability
- Identify edge cases and bugs

## Documentation Requirements

1. **User Guide**
   - Installation instructions
   - How to use copy/paste features
   - Keyboard shortcuts
   - Troubleshooting

2. **Developer Documentation**
   - Architecture overview
   - API documentation
   - Extension points for custom nodes
   - Contribution guidelines

3. **JSON Schema Documentation**
   - Complete schema reference
   - Examples
   - Version history

## Future Enhancements

- **Diff Tool**: Compare two Blueprint graphs
- **Merge Tool**: Merge changes from JSON
- **External Editor**: Edit JSON in external tools
- **Version Control Integration**: Git-friendly JSON format
- **AI Integration**: Generate Blueprint graphs from JSON descriptions
- **Batch Operations**: Export/import multiple graphs
- **Template Library**: Share common graph patterns

## Resources & References

### Unreal Engine Documentation
- [Plugin Development Guide](https://docs.unrealengine.com/5.3/en-US/plugins-in-unreal-engine/)
- [Blueprint API Reference](https://docs.unrealengine.com/5.3/en-US/blueprint-api-in-unreal-engine/)
- [Editor Extensions](https://docs.unrealengine.com/5.3/en-US/extending-the-editor-in-unreal-engine/)

### Existing Tools (Reference)
- BPtoJSON Plugin
- Blueprint Node Exporter
- JsonBlueprint Plugin
- GraphPrinter Plugin

### Community Resources
- Unreal Engine Forums
- Unreal Engine Discord
- GitHub repositories with similar functionality

## Next Steps

1. **Review this roadmap** and adjust based on priorities
2. **Set up development environment** (Unreal Engine, IDE)
3. **Create plugin project structure**
4. **Start Phase 1** - Foundation & Research
5. **Create initial prototype** to validate approach

## Questions to Resolve

Before starting development, consider:

1. **Target Unreal Engine Version**: Which version(s) should we support?
2. **Node Type Coverage**: Which node types are priority?
3. **UI Preference**: Context menu, toolbar, or both?
4. **Distribution**: Marketplace, GitHub, or both?
5. **License**: What license should the project use?

---

**Last Updated**: 2024-01-01
**Status**: Planning Phase

