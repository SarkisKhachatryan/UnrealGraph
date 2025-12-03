# Quick Start Guide

This guide will help you get started with the Unreal Graph project.

## Overview

Unreal Graph is a plugin that allows copying Blueprint graphs as JSON and pasting them back into Unreal Editor.

## Current Status

🚧 **Project Status**: Planning Phase

The project is currently in the planning and research phase. Development has not yet begun.

## Getting Started

### Step 1: Review Documentation

Before starting development, review these documents:

1. **README.md** - Project overview
2. **ROADMAP.md** - Complete development roadmap
3. **TECHNICAL_RESEARCH.md** - Technical research and API information
4. **PROJECT_STRUCTURE.md** - Proposed file structure
5. **PRE_DEVELOPMENT_CHECKLIST.md** - Questions to answer before starting

### Step 2: Answer Pre-Development Questions

Review `PRE_DEVELOPMENT_CHECKLIST.md` and answer the key questions:
- Target Unreal Engine version
- Development platform
- Feature priorities
- Distribution method

### Step 3: Set Up Development Environment

#### Required Software

1. **Unreal Engine 5.0+**
   - Download from Epic Games Launcher
   - Or get source code from GitHub (recommended for plugin development)

2. **IDE**
   - Visual Studio 2022 (required)
   - **Alternative**: JetBrains Rider for Unreal

3. **Git** (for version control)

#### Setup Steps

1. **Install Unreal Engine**
   ```bash
   # Via Epic Games Launcher
   # Or clone from GitHub (requires Epic Games account)
   ```

2. **Generate Project Files**
   ```batch
   REM Navigate to Unreal Engine directory
   cd C:\Program Files\Epic Games\UE_5.3\Engine
   
   REM Generate project files using UnrealVersionSelector
   REM Right-click on .uproject file and select "Generate Visual Studio project files"
   REM Or use UnrealVersionSelector.exe from command line
   ```

3. **Create Plugin Directory**
   ```batch
   REM Create plugin in your project or engine plugins folder
   mkdir YourProject\Plugins\UnrealGraph
   REM Or
   mkdir Engine\Plugins\UnrealGraph
   ```

### Step 4: Create Initial Plugin Structure

Once you're ready to start development:

1. **Create Plugin Descriptor**
   - Create `UnrealGraph.uplugin` file
   - Define plugin metadata

2. **Create Module Files**
   - Create `Source/UnrealGraph/` directory
   - Create `UnrealGraph.Build.cs`
   - Create basic module files

3. **Test Plugin Loading**
   - Compile plugin
   - Enable in Unreal Editor
   - Verify it loads without errors

### Step 5: Start Development

Follow the roadmap in `ROADMAP.md`:

1. **Phase 1**: Foundation & Research
   - Set up plugin structure
   - Research Blueprint APIs
   - Create test Blueprints

2. **Phase 2**: Serialization
   - Implement graph-to-JSON conversion

3. **Phase 3**: Deserialization
   - Implement JSON-to-graph conversion

4. **Phase 4**: Editor Integration
   - Add UI and shortcuts

5. **Phase 5**: Advanced Features
   - Handle complex scenarios

6. **Phase 6**: Testing & Polish
   - Test and document

## Development Workflow

### Daily Workflow

1. **Research** (if needed)
   - Study Unreal Engine APIs
   - Review existing plugins
   - Test concepts in small prototypes

2. **Implement**
   - Write code following the roadmap
   - Test incrementally
   - Document as you go

3. **Test**
   - Test with various Blueprint types
   - Test edge cases
   - Fix bugs

4. **Iterate**
   - Refine implementation
   - Optimize performance
   - Improve user experience

### Testing Workflow

1. **Create Test Blueprints**
   - Simple graphs (few nodes)
   - Complex graphs (many nodes, loops)
   - Various node types
   - Different Blueprint types

2. **Test Serialization**
   - Export to JSON
   - Verify JSON structure
   - Check data accuracy

3. **Test Deserialization**
   - Import from JSON
   - Verify graph reconstruction
   - Check functionality

4. **Test Integration**
   - Test context menu
   - Test keyboard shortcuts
   - Test clipboard operations

## Useful Resources

### Unreal Engine Documentation
- [Plugin Development](https://docs.unrealengine.com/5.3/en-US/plugins-in-unreal-engine/)
- [Blueprint API](https://docs.unrealengine.com/5.3/en-US/blueprint-api-in-unreal-engine/)
- [Editor Extensions](https://docs.unrealengine.com/5.3/en-US/extending-the-editor-in-unreal-engine/)

### Unreal Engine Source Code
- [GitHub Repository](https://github.com/EpicGames/UnrealEngine)
- Key directories:
  - `Engine/Source/Editor/GraphEditor/` - Graph editor code
  - `Engine/Source/Editor/BlueprintGraph/` - Blueprint graph code
  - `Engine/Source/Runtime/Json/` - JSON serialization

### Community Resources
- [Unreal Engine Forums](https://forums.unrealengine.com/)
- [Unreal Engine Discord](https://discord.gg/unrealengine)
- [Unreal Engine Reddit](https://www.reddit.com/r/unrealengine/)

### Reference Plugins
- BPtoJSON - Similar functionality (reference implementation)
- GraphPrinter - Graph manipulation example
- JsonBlueprint - JSON handling in Blueprints

## Common Issues & Solutions

### Issue: Plugin won't compile
**Solution**: 
- Check module dependencies in `.Build.cs`
- Verify Unreal Engine version compatibility
- Check include paths

### Issue: Plugin loads but doesn't appear in editor
**Solution**:
- Check plugin is enabled in project settings
- Verify module is loaded correctly
- Check for errors in output log

### Issue: Can't access Blueprint graph
**Solution**:
- Verify you're in Blueprint editor context
- Check Blueprint is not compiled-only
- Ensure proper editor module setup

### Issue: JSON serialization fails
**Solution**:
- Check JSON library is included
- Verify FJsonObject usage
- Check for null pointers

## Next Steps

1. **Review all documentation** in this repository
2. **Answer pre-development questions**
3. **Set up development environment**
4. **Create initial plugin structure**
5. **Start Phase 1** of the roadmap

## Getting Help

If you need help:

1. **Review documentation** - Check all `.md` files
2. **Research** - Look up Unreal Engine APIs
3. **Community** - Ask on Unreal Engine forums/Discord
4. **Examples** - Study existing plugins
5. **Experiment** - Create small test projects

## Contributing

Once development begins, contributions are welcome! See the roadmap for areas where help is needed.

---

**Remember**: This is a complex project. Take your time, research thoroughly, and test incrementally. Good luck!

