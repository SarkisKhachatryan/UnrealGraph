# Unreal Graph - Project Summary

## Project Overview

**Unreal Graph** is a Unreal Engine plugin that enables developers to copy Blueprint graphs from the Unreal Editor, convert them to JSON format, and paste them back into the editor to reconstruct the graph.

## Project Goals

1. **Copy Blueprint graphs** as JSON from Unreal Editor
2. **Store graphs** in JSON format for version control, sharing, or external processing
3. **Paste JSON** back into Unreal Editor to recreate Blueprint graphs
4. **Preserve** node positions, connections, and properties

## Current Status

🚧 **Status**: Planning Phase

The project is currently in the planning and research phase. All documentation has been created, but development has not yet begun.

## Documentation Structure

This repository contains comprehensive planning documentation:

### Core Documents

1. **README.md**
   - Project overview
   - Basic information
   - Quick links

2. **ROADMAP.md**
   - Complete development roadmap
   - 6 development phases
   - 12-week timeline
   - Technical architecture
   - JSON schema design
   - Implementation details

3. **TECHNICAL_RESEARCH.md**
   - Unreal Engine API research
   - Blueprint system architecture
   - Key classes and methods
   - Serialization approaches
   - Editor integration points
   - Challenges and solutions

4. **PROJECT_STRUCTURE.md**
   - Proposed file structure
   - Directory organization
   - File descriptions
   - Build system integration

5. **PRE_DEVELOPMENT_CHECKLIST.md**
   - Questions to answer before starting
   - Technical decisions needed
   - Scope definition
   - Risk assessment

6. **QUICK_START.md**
   - Getting started guide
   - Setup instructions
   - Development workflow
   - Common issues

7. **PROJECT_SUMMARY.md** (this file)
   - High-level overview
   - Document index
   - Quick reference

## Development Phases

### Phase 1: Foundation & Research (Week 1-2)
- Set up plugin structure
- Research Blueprint APIs
- Create test Blueprints

### Phase 2: Serialization (Week 3-4)
- Implement graph-to-JSON conversion
- Extract node data and connections
- Handle various node types

### Phase 3: Deserialization (Week 5-6)
- Implement JSON-to-graph conversion
- Create nodes from JSON
- Establish connections

### Phase 4: Editor Integration (Week 7-8)
- Add context menu items
- Implement keyboard shortcuts
- Clipboard integration

### Phase 5: Advanced Features (Week 9-10)
- Support custom nodes
- Handle macros and nested graphs
- Version migration

### Phase 6: Testing & Polish (Week 11-12)
- Comprehensive testing
- Performance optimization
- Documentation

## Technical Architecture

### Core Components

1. **FBlueprintGraphSerializer**
   - Traverses Blueprint graphs
   - Converts to JSON format

2. **FBlueprintGraphDeserializer**
   - Parses JSON
   - Reconstructs graphs

3. **FUnrealGraphEditorModule**
   - Editor integration
   - UI and shortcuts

4. **FBlueprintGraphJsonSchema**
   - Schema validation
   - Version management

### Key Technologies

- **C++** - Primary development language
- **Unreal Engine Plugin System** - Plugin architecture
- **Unreal Engine Blueprint API** - Graph manipulation
- **Unreal Engine JSON API** - Serialization
- **Unreal Engine Editor Extensions** - UI integration

## JSON Schema

The JSON format includes:
- **Metadata**: Version, timestamp, Blueprint type
- **Nodes**: Type, position, properties, pins
- **Connections**: Source/target nodes and pins
- **Comments**: Text, position, associated nodes
- **Variables**: Variable definitions

## Requirements

### Prerequisites
- Unreal Engine 5.0+ (recommended: 5.3+)
- Visual Studio 2022 (Windows)
- Unreal Engine source code access (recommended)

### Knowledge Areas
- Unreal Engine plugin development
- Blueprint graph system internals
- JSON serialization/deserialization
- Unreal Engine editor extensions

## Challenges

### Technical Challenges
1. **Node Type Mapping** - Different node classes need different handling
2. **Pin Type Serialization** - Complex types need special handling
3. **Graph State Management** - Undo/redo support
4. **Performance** - Large graphs may be slow
5. **Version Compatibility** - Different UE versions

### Solutions
- Factory pattern for node serialization
- Unreal's property system for complex types
- Transaction system for state management
- Async operations for performance
- Version schema and migration

## Next Steps

1. **Review Documentation**
   - Read all planning documents
   - Understand the roadmap
   - Review technical research

2. **Answer Pre-Development Questions**
   - Target Unreal Engine version
   - Development platform
   - Feature priorities
   - Distribution method

3. **Set Up Development Environment**
   - Install Unreal Engine
   - Set up IDE
   - Get source code access

4. **Create Initial Plugin Structure**
   - Create plugin descriptor
   - Set up module files
   - Test plugin loading

5. **Start Phase 1**
   - Begin foundation work
   - Start research phase
   - Create prototypes

## Resources

### Documentation
- All planning documents in this repository
- Unreal Engine official documentation
- Technical research notes

### Community
- Unreal Engine Forums
- Unreal Engine Discord
- GitHub repositories

### Reference Plugins
- BPtoJSON
- GraphPrinter
- JsonBlueprint

## Project Timeline

**Estimated Duration**: 12 weeks (part-time) or 6 weeks (full-time)

**Current Phase**: Planning (Week 0)

**Next Phase**: Foundation & Research (Week 1-2)

## Success Criteria

The project will be considered successful when:

1. ✅ Users can copy Blueprint graphs as JSON
2. ✅ Users can paste JSON to recreate graphs
3. ✅ Node positions and connections are preserved
4. ✅ Plugin integrates seamlessly with Unreal Editor
5. ✅ Works with common node types
6. ✅ Handles errors gracefully
7. ✅ Well-documented and tested

## Questions?

If you have questions:

1. Review the relevant documentation
2. Check `PRE_DEVELOPMENT_CHECKLIST.md` for common questions
3. Review `TECHNICAL_RESEARCH.md` for technical details
4. Consult Unreal Engine documentation
5. Ask the community

## Getting Started

Ready to start? Follow these steps:

1. Read `QUICK_START.md` for setup instructions
2. Review `ROADMAP.md` for the development plan
3. Answer questions in `PRE_DEVELOPMENT_CHECKLIST.md`
4. Set up your development environment
5. Begin Phase 1 of the roadmap

---

**Good luck with your project!** 🚀

This is an ambitious project, but with careful planning and incremental development, it's definitely achievable. Take your time, research thoroughly, and test incrementally.

