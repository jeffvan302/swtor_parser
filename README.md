# SWTOR Combat Parser

A high-performance, extensible combat log parser for Star Wars: The Old Republic (SWTOR) with plugin support and NTP time synchronization.

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [Features](#features)
- [Project Structure](#project-structure)
- [Quick Start](#quick-start)
- [Building](#building)
- [Usage](#usage)
- [Plugin Development](#plugin-development)
- [Documentation](#documentation)
- [Requirements](#requirements)

## Overview

The SWTOR Combat Parser is a C++20 library and application suite designed to parse SWTOR combat log files with high performance and extensibility. The project provides two implementation approaches:

1. **Direct Integration** - Compile the core parser directly into your executable with custom plugins
2. **DLL-based Architecture** - Use the parser as a DLL with dynamically loadable external plugins

## Architecture

### Core Library (`swtor_parser_core`)

The heart of the project, providing:
- Combat log parsing engine
- Event processing pipeline
- Plugin management system
- Combat state tracking
- NTP time synchronization
- Entity tracking and statistics

### Two Implementation Approaches

#### 1. Direct Integration (`swtor_parser.sln`)

**Location:** `example_swtor_parser/`

Compiles the core parser directly into a single executable along with your custom plugins.

**Advantages:**
- Simple deployment (single executable)
- Best performance (no DLL overhead)
- Easier debugging
- Direct access to all parser internals

**Use when:**
- Building a standalone application
- Performance is critical
- You want simple deployment
- You have a fixed set of plugins

#### 2. DLL-based Architecture (`swtor_parser_dll_hosting.sln`)

**Location:** `host_parsing/` + `example_plugin/`

Builds the core as a DLL (`swtor_parser_core.dll`) that can load external plugin DLLs dynamically at runtime.

**Advantages:**
- Plugin hot-loading without recompiling
- Modular architecture
- Third-party plugin support
- Update plugins independently

**Use when:**
- Building a plugin ecosystem
- Need runtime plugin loading
- Want to distribute plugins separately
- Multiple developers creating plugins

## Features

### Core Parsing Features

- **High Performance**: Processes thousands of combat lines per second
- **Complete Log Parsing**: Handles all SWTOR combat log event types
- **Combat State Tracking**: Automatically detects combat start/end, area transitions
- **Entity Management**: Tracks all entities (players, NPCs, companions) with stats
- **Event Processing**: Damage, healing, threat, deaths, and more
- **Time Synchronization**: NTP-based precise timestamps with timezone support
- **Memory Efficient**: Optimized memory usage for large log files

### Plugin System

- **Priority-based Execution**: Control plugin execution order
- **Shared State**: Access to combat state, entities, and parse data
- **Event Filtering**: Process only relevant events
- **Reset Handling**: Proper cleanup on combat end or parser reset
- **API Versioning**: Ensures plugin compatibility

### Timing Features

- **NTP Synchronization**: Sync with network time protocol servers
- **Time Cruncher**: Advanced timestamp processing with late-arrival adjustment
- **Timezone Support**: Local time conversion
- **Timing Plugin**: Simulate real-time log playback with speed control

## Project Structure

```
swtor_parser/
├── core/                              # Core parser library (missing from archive)
│   ├── swtor_parser_core.vcxproj     # Core library project
│   ├── swtor_parser.cpp/.h           # Main parser implementation
│   ├── parse_manager.cpp/.h          # Plugin manager and processing pipeline
│   ├── combat_state.cpp/.h           # Combat state tracking
│   ├── ntp_timekeeper_swtor.cpp/.h   # NTP time synchronization
│   ├── time_cruncher_swtor.cpp/.h    # Timestamp processing
│   ├── timing_plugin.cpp/.h          # Built-in timing plugin
│   ├── parse_plugin.h                # Plugin base class
│   ├── plugin_api.h                  # External plugin API
│   ├── base_classes.h                # Common data structures
│   ├── stat_keeper.h                 # Statistics tracking
│   └── memory_helper.h               # Memory utilities
│
├── example_swtor_parser/             # Direct integration example
│   ├── swtor_parser.vcxproj          # Direct integration project
│   ├── example_swtor_integration.cpp # Main application with integrated parser
│   ├── test_plugin.cpp/.h            # Example built-in plugin
│   └── swtor_parser.sln              # Direct integration solution
│
├── host_parsing/                     # DLL host application
│   ├── parser_host.vcxproj           # Host executable project
│   ├── parser_host.cpp               # DLL loading and plugin management
│   └── app_plugin.cpp/.h             # Application-specific plugin
│
├── example_plugin/                   # External plugin example
│   ├── example_damage_tracker_plugin.vcxproj  # External plugin project
│   ├── example_damage_tracker_plugin.cpp      # Plugin implementation
│   └── plugin_sdk.h                           # Plugin SDK header
│
├── Documentation/                    # Technical documentation
│   └── Star Wars_ The Old Republic Combat Log Technical Reference.pdf
│
├── swtor_parser_dll_hosting.sln     # DLL-based solution
└── README.md                         # This file
```

## Quick Start

### Option 1: Using the Direct Integration

1. Open `swtor_parser.sln` in Visual Studio 2022
2. Build the solution (Release x64 recommended)
3. Run the executable:
   ```bash
   swtor_parser.exe combat_log_file.txt
   ```

### Option 2: Using the DLL Architecture

1. Open `swtor_parser_dll_hosting.sln` in Visual Studio 2022
2. Build all projects (builds core DLL, host executable, and example plugin)
3. Create a `plugins` directory next to `parser_host.exe`
4. Copy plugin DLLs to the `plugins` directory
5. Run the host:
   ```bash
   parser_host.exe combat_log_file.txt
   ```

## Building

### Requirements

- **Visual Studio 2022** (v143 platform toolset)
- **Windows SDK 10.0** or later
- **C++20** standard support
- **Windows 10/11** (for file system and networking APIs)

### Build Configurations

Both solutions support:
- **Debug|x64**: Development build with debug symbols
- **Release|x64**: Optimized production build

### Build Steps

#### For Direct Integration:

1. Open `swtor_parser.sln`
2. Select **Release|x64** configuration
3. Build → Build Solution
4. Output: `output/Release/swtor_parser.exe`

#### For DLL Architecture:

1. Open `swtor_parser_dll_hosting.sln`
2. Select **Release|x64** configuration
3. Build → Build Solution
4. Output files:
   - `output/Release/swtor_parser_core.dll` (core library)
   - `output/Release/swtor_parser_core.lib` (import library)
   - `output/Release/parser_host.exe` (host application)
   - `output/Release/example_damage_tracker_plugin.dll` (example plugin)

## Usage

### Basic Usage (Direct Integration)

```bash
# Process a combat log file
swtor_parser.exe combat_log.txt

# No verbose output
swtor_parser.exe combat_log.txt no

# Adjust playback speed (300x faster)
swtor_parser.exe combat_log.txt no 300.0

# Speed processing (no timing plugin)
swtor_parser.exe combat_log.txt no 0.0
```

### Basic Usage (DLL Host)

```bash
# Process with plugins from 'plugins' directory
parser_host.exe combat_log.txt
```

### Programmatic Usage

```cpp
#include "../core/parse_manager.h"

// Create plugin manager
swtor::plugin_manager manager;

// Register your plugin
auto my_plugin = std::make_shared<MyPlugin>();
manager.register_plugin(my_plugin);

// Process log lines
for (const auto& line : log_lines) {
    manager.process_line(line);
    
    // Check combat state
    if (manager.is_in_combat()) {
        // Access combat data
        auto combat_time = manager.get_combat_state().get_combat_time();
        // ... process data
    }
}
```

## Plugin Development

### Creating a Built-in Plugin (Direct Integration)

1. Create a class inheriting from `swtor::parse_plugin`:

```cpp
#include "../core/parse_manager.h"

class MyPlugin : public swtor::parse_plugin {
public:
    std::string name() const override {
        return "MyPlugin";
    }
    
    void ingest(swtor::parse_data_holder& parse_data, 
                swtor::CombatLine& line) override {
        // Process combat line
        if (line.event.type_id == swtor::KINDID_Event) {
            // Handle events
        }
    }
    
    void reset() override {
        // Reset plugin state
    }
};
```

2. Register with the plugin manager:

```cpp
auto plugin = std::make_shared<MyPlugin>();
plugin->set_priority(10); // Optional: set execution priority
manager.register_plugin(plugin);
```

### Creating an External Plugin (DLL)

1. Create a new DLL project in Visual Studio
2. Include the plugin SDK:

```cpp
#include "plugin_sdk.h"

class MyExternalPlugin : public swtor::ExternalPluginBase {
public:
    std::string name() const override {
        return "MyExternalPlugin";
    }
    
    PluginInfo GetInfo() const override {
        return {
            "MyExternalPlugin",
            "1.0.0",
            "Your Name",
            "Plugin description",
            PLUGIN_API_VERSION
        };
    }
    
    void ingest(swtor::parse_data_holder& parse_data,
                swtor::CombatLine& line) override {
        // Process combat line
    }
    
    void reset() override {
        // Reset state
    }
};

EXPORT_PLUGIN(MyExternalPlugin)
```

3. Configure project settings:
   - Configuration Type: Dynamic Library (.dll)
   - C++ Language Standard: C++20
   - Link against `swtor_parser_core.lib`

4. Place the compiled DLL in the `plugins` directory

See `PLUGIN_DEVELOPMENT.md` for detailed plugin development guide.

## Documentation

- **README.md** (this file) - Project overview and quick start
- **PLUGIN_DEVELOPMENT.md** - Detailed plugin development guide
- **BUILD_INSTRUCTIONS.md** - Comprehensive build instructions
- **API_REFERENCE.md** - Core API documentation
- **Documentation/Star Wars_ The Old Republic Combat Log Technical Reference.pdf** - SWTOR log format specification

## Combat Log Format

SWTOR combat logs have the format:
```
[HH:MM:SS.mmm] [@SourceName] [EffectName {EffectID}] [Event {EventID}] <TargetInfo> = Value
```

Example:
```
[23:45:12.345] [@PlayerName] [Force Lightning {1234567890}] [Damage {836045448945500}] [@TargetNPC] *350*
```

The parser handles:
- Timestamps with millisecond precision
- Entity identification (players, NPCs, companions)
- Ability/effect tracking with unique IDs
- Event types (damage, healing, threat, death, etc.)
- Value information (amounts, percentages, critical hits)
- Combat state transitions

## Performance

Typical performance metrics:
- **Parsing Speed**: 20,000-50,000 lines/second (Release build)
- **Memory Usage**: ~100-500MB for typical raid logs
- **Latency**: <0.05ms average per line

Performance varies based on:
- Number and complexity of registered plugins
- Log file size and event density
- System specifications

## Examples

The project includes several example implementations:

1. **TestPlugin** (`example_swtor_parser/test_plugin.cpp`)
   - Simple DPS/HPS calculator
   - Demonstrates basic plugin structure
   - Shows combat state tracking

2. **DamageTrackerPlugin** (`example_plugin/example_damage_tracker_plugin.cpp`)
   - External DLL plugin example
   - Tracks damage by entity
   - Demonstrates external plugin API

3. **TimingPlugin** (built-in)
   - Real-time log playback simulation
   - Adjustable playback speed
   - Combat/non-combat timing modes

## Troubleshooting

### Missing core/ directory

If you get build errors about missing core files, ensure the `core/` directory exists with all the parser library source files. The core library should contain:
- swtor_parser.cpp/.h
- parse_manager.cpp/.h
- combat_state.cpp/.h
- And other core files listed in the project structure

### Plugin not loading

1. Ensure plugin DLL is in the `plugins` directory
2. Check that `swtor_parser_core.dll` is in the same directory as `parser_host.exe`
3. Verify plugin API version matches: `PLUGIN_API_VERSION`
4. Check console output for error messages

### Performance issues

1. Use Release build configuration
2. Disable verbose output for large files
3. Reduce number of active plugins
4. Ensure adequate system memory

## Contributing

When contributing plugins or improvements:

1. Follow the existing code style
2. Ensure C++20 compatibility
3. Test with both Debug and Release builds
4. Document public APIs
5. Handle errors gracefully

## License

[Specify your license here]

## Acknowledgments

- SWTOR combat log format documentation
- NTP time synchronization libraries
- Plugin architecture inspired by industry best practices

## Contact

[Your contact information]

---

**Note**: This parser is designed for SWTOR combat log analysis and is not affiliated with EA or BioWare.
