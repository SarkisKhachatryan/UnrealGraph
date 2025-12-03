# Project Structure

This document outlines the proposed file structure for the Unreal Graph plugin.

## Plugin Structure

```
UnrealGraph/
├── README.md                          # Project overview
├── ROADMAP.md                         # Development roadmap
├── TECHNICAL_RESEARCH.md              # Technical research notes
├── PROJECT_STRUCTURE.md               # This file
├── PRE_DEVELOPMENT_CHECKLIST.md       # Pre-development questions
│
├── Source/                            # C++ source code
│   └── UnrealGraph/
│       ├── UnrealGraph.Build.cs       # Build configuration
│       ├── UnrealGraph.cpp            # Module implementation
│       ├── UnrealGraph.h               # Module header
│       │
│       ├── Public/                    # Public headers
│       │   ├── UnrealGraphEditorModule.h
│       │   ├── BlueprintGraphSerializer.h
│       │   ├── BlueprintGraphDeserializer.h
│       │   ├── BlueprintGraphJsonSchema.h
│       │   ├── UnrealGraphCommands.h
│       │   └── UnrealGraphStyle.h
│       │
│       └── Private/                   # Private implementation
│           ├── UnrealGraphEditorModule.cpp
│           ├── BlueprintGraphSerializer.cpp
│           ├── BlueprintGraphDeserializer.cpp
│           ├── BlueprintGraphJsonSchema.cpp
│           ├── UnrealGraphCommands.cpp
│           └── UnrealGraphStyle.cpp
│
├── Resources/                         # Plugin resources
│   ├── Icon128.png                    # Plugin icon
│   └── Icon256.png                    # Large plugin icon
│
├── Content/                           # Blueprint/asset content (if needed)
│   └── EditorUtilities/               # Editor utility widgets (optional)
│
├── Config/                            # Configuration files
│   └── DefaultUnrealGraph.ini         # Default settings
│
└── UnrealGraph.uplugin               # Plugin descriptor file
```

## File Descriptions

### Core Module Files

#### UnrealGraph.uplugin
Plugin descriptor file that defines:
- Plugin name, version, description
- Module dependencies
- Supported engine versions
- Loading phase

#### UnrealGraph.Build.cs
Build configuration file that specifies:
- Module dependencies (Core, CoreUObject, Engine, Editor, etc.)
- Include paths
- Library dependencies

#### UnrealGraph.h / UnrealGraph.cpp
Main module implementation:
- Module startup/shutdown
- Plugin initialization
- Global module state

### Serialization Classes

#### BlueprintGraphSerializer.h / .cpp
**Purpose**: Convert Blueprint graphs to JSON

**Key Methods**:
- `SerializeGraph(UEdGraph*)` - Serialize entire graph
- `SerializeNode(UEdGraphNode*)` - Serialize single node
- `SerializePin(UEdGraphPin*)` - Serialize pin
- `SerializeConnections(UEdGraph*)` - Serialize all connections

**Dependencies**:
- UEdGraph, UEdGraphNode, UEdGraphPin
- FJsonObject, FJsonValue

#### BlueprintGraphDeserializer.h / .cpp
**Purpose**: Convert JSON to Blueprint graphs

**Key Methods**:
- `DeserializeGraph(UEdGraph*, FJsonObject*)` - Deserialize entire graph
- `CreateNodeFromJson(UEdGraph*, FJsonObject*)` - Create node from JSON
- `CreateConnectionsFromJson(UEdGraph*, FJsonArray*)` - Create connections
- `ValidateJsonSchema(FJsonObject*)` - Validate JSON structure

**Dependencies**:
- FGraphNodeCreator
- FBlueprintEditorUtils
- FScopedTransaction

#### BlueprintGraphJsonSchema.h / .cpp
**Purpose**: JSON schema definition and validation

**Key Methods**:
- `GetCurrentSchemaVersion()` - Get schema version
- `ValidateJson(FJsonObject*)` - Validate JSON against schema
- `MigrateJson(FJsonObject*, int32 FromVersion)` - Migrate old schemas

**Dependencies**:
- FJsonObject

### Editor Integration

#### UnrealGraphEditorModule.h / .cpp
**Purpose**: Editor module and UI integration

**Key Methods**:
- `StartupModule()` - Initialize plugin
- `ShutdownModule()` - Cleanup plugin
- `ExtendBlueprintEditorMenu()` - Add context menu items
- `RegisterCommands()` - Register keyboard shortcuts
- `OnCopyAsJSON()` - Handle copy action
- `OnPasteFromJSON()` - Handle paste action

**Dependencies**:
- FBlueprintEditorModule
- FExtensibilityManager
- FUICommandList

#### UnrealGraphCommands.h / .cpp
**Purpose**: Define UI commands and keyboard shortcuts

**Key Definitions**:
- `FUnrealGraphCommands` - Command set
- `CopyAsJSON` - Copy command
- `PasteFromJSON` - Paste command

#### UnrealGraphStyle.h / .cpp
**Purpose**: Define UI styles and icons (optional)

**Key Methods**:
- `Initialize()` - Initialize styles
- `Shutdown()` - Cleanup styles
- `GetIcon()` - Get icon resources

## Directory Structure Rationale

### Source/UnrealGraph/
Main plugin source code directory. Follows Unreal Engine plugin conventions.

### Public/ vs Private/
- **Public/**: Headers that may be included by other modules
- **Private/**: Implementation files and internal headers

### Resources/
Contains plugin icons and other non-code resources.

### Content/
Optional directory for Blueprint assets or editor utility widgets if needed.

### Config/
Configuration files for plugin settings.

## Build System Integration

The plugin will integrate with Unreal Engine's build system:
- Uses `.Build.cs` files for module configuration
- Automatically compiled when engine builds
- Can be hot-reloaded during development

## Testing Structure (Future)

When unit tests are added:
```
Source/
└── UnrealGraphTests/
    ├── UnrealGraphTests.Build.cs
    ├── Public/
    │   └── BlueprintGraphSerializerTests.h
    └── Private/
        └── BlueprintGraphSerializerTests.cpp
```

## Documentation Structure

Documentation files in root:
- `README.md` - Project overview
- `ROADMAP.md` - Development plan
- `TECHNICAL_RESEARCH.md` - Technical notes
- `PROJECT_STRUCTURE.md` - This file
- `PRE_DEVELOPMENT_CHECKLIST.md` - Pre-dev checklist

## Version Control

Recommended `.gitignore` entries:
```
# Unreal Engine
Binaries/
Intermediate/
Saved/
DerivedDataCache/
*.sln
*.suo
*.sdf
*.opensdf
*.db
*.opendb

# Visual Studio
.vs/
*.user
*.userosscache

# Build artifacts
*.obj
*.pch
*.pdb
```

## Notes

- All file paths are relative to the plugin root directory
- Structure follows Unreal Engine plugin conventions
- Can be adjusted based on actual implementation needs
- Additional directories can be added as needed

