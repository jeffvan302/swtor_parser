# SWTOR Parser - Quick Start Guide

Get up and running with the SWTOR Combat Parser in under 5 minutes.

## What You Need

- Visual Studio 2022
- Windows 10 or later
- A SWTOR combat log file

## Step 1: Choose Your Approach

### Option A: Simple Executable (Recommended for beginners)

Best for:
- Quick testing
- Single application
- Simple deployment

**Time**: 2 minutes

### Option B: Plugin Architecture

Best for:
- Extensibility
- Multiple plugins
- Learning the system

**Time**: 5 minutes

---

## Option A: Simple Executable

### 1. Open the Project

```bash
# Double-click this file:
swtor_parser.sln
```

### 2. Build

In Visual Studio:
1. Select **Release | x64** from the dropdown
2. Press **Ctrl+Shift+B** (Build Solution)
3. Wait ~30 seconds for build to complete

### 3. Run

```bash
# Navigate to output
cd output\Release

# Run with your combat log
swtor_parser.exe C:\Path\To\Your\combat_log.txt
```

### 4. See Results

You'll see output like:
```
Parsing Test File: combat_log.txt
=== Entering Combat ===
Area: The Red Reaper
DPS: 12543.67  HP%: 85.23%
...
=== Exiting Combat ===
Combat Time: 3m 45s
Final DPS: 13127.89
```

**That's it!** You're parsing logs.

---

## Option B: Plugin Architecture

### 1. Open the Project

```bash
# Double-click this file:
swtor_parser_dll_hosting.sln
```

### 2. Build All

In Visual Studio:
1. Select **Release | x64**
2. Press **Ctrl+Shift+B**
3. Wait ~1 minute for all projects to build

### 3. Create Plugin Directory

```bash
cd output\Release
mkdir plugins
```

### 4. Copy Plugin

```bash
# Copy the example plugin to plugins folder
copy example_damage_tracker_plugin.dll plugins\
```

### 5. Run

```bash
parser_host.exe C:\Path\To\Your\combat_log.txt
```

### 6. See Results

```
SWTOR Combat Parser Host Application
=====================================
Parser host initialized successfully
Scanning for plugins in: plugins
Loading plugin: example_damage_tracker_plugin.dll
Loaded 1 external plugins

[DamageTracker] Entered combat
[DamageTracker] Total damage: 125000 from 8 entities
...
```

**Done!** You're using plugins.

---

## Quick Customization

### Change Playback Speed

```bash
# 10x faster playback
swtor_parser.exe combat_log.txt no 10.0

# 100x faster
swtor_parser.exe combat_log.txt no 100.0

# Full speed (no delay)
swtor_parser.exe combat_log.txt no 0.0
```

### Disable Verbose Output

```bash
# Quiet mode
swtor_parser.exe combat_log.txt no
```

---

## Create Your First Plugin

### Built-in Plugin (5 minutes)

1. **Create files**:
   - `my_plugin.h`
   - `my_plugin.cpp`

2. **Implement** (`my_plugin.h`):

```cpp
#pragma once
#include "../core/parse_manager.h"

namespace swtor {

class MyPlugin : public parse_plugin {
public:
    std::string name() const override {
        return "MyPlugin";
    }
    
    void ingest(parse_data_holder& parse_data, CombatLine& line) override {
        // Count damage events
        if (line.event.type_id == KINDID_Event) {
            if (line.event.action_id == 
                static_cast<uint64_t>(EventActionType::Damage)) {
                damage_count_++;
            }
        }
    }
    
    void reset() override {
        damage_count_ = 0;
    }
    
    int get_count() const { return damage_count_; }
    
private:
    int damage_count_ = 0;
};

} // namespace swtor
```

3. **Add to project**:
   - Right-click `example_swtor_parser` project
   - Add → Existing Item → Select `my_plugin.h` and `my_plugin.cpp`

4. **Use it** (in `example_swtor_integration.cpp`):

```cpp
#include "my_plugin.h"

// In main() or wherever you create plugin_manager:
auto my_plugin = std::make_shared<swtor::MyPlugin>();
manager.register_plugin(my_plugin);

// After processing:
std::cout << "Total damage events: " << my_plugin->get_count() << std::endl;
```

5. **Rebuild and run!**

---

## Common Issues

### "Cannot find swtor_parser_core.lib"

**Problem**: Building a plugin but core library not built yet.

**Fix**: 
```bash
# Build core first
Right-click swtor_parser_core project → Build
# Then build your plugin
```

### "DLL not found"

**Problem**: Host can't find core DLL.

**Fix**:
```bash
# Ensure both are in same directory
copy swtor_parser_core.dll [directory_with_parser_host.exe]
```

### "Plugin not loading"

**Problem**: Plugin DLL not in plugins folder.

**Fix**:
```bash
# Check plugin location
dir plugins
# Should show: example_damage_tracker_plugin.dll
```

---

## Next Steps

### Learn More

1. **Read README.md** - Full project overview
2. **Read PLUGIN_DEVELOPMENT.md** - Detailed plugin guide
3. **Read API_REFERENCE.md** - Complete API docs
4. **Study examples**:
   - `test_plugin.cpp` - Built-in plugin example
   - `example_damage_tracker_plugin.cpp` - DLL plugin example

### Experiment

Try modifying the example plugins:
- Track healing instead of damage
- Count critical hits
- Monitor specific abilities
- Track entity deaths

### Build Something Cool

Ideas:
- **DPS Meter**: Track personal DPS
- **Rotation Analyzer**: Analyze ability usage
- **Combat Logger**: Save combat summaries
- **Stats Tracker**: Track performance over time
- **Boss Analyzer**: Identify boss mechanics

---

## Command Reference

### Direct Integration (swtor_parser.exe)

```bash
# Basic usage
swtor_parser.exe <log_file>

# No verbose output
swtor_parser.exe <log_file> no

# Adjust speed (1.0 = real-time, 100.0 = 100x faster)
swtor_parser.exe <log_file> no <speed_factor>

# Examples:
swtor_parser.exe combat.txt
swtor_parser.exe combat.txt no
swtor_parser.exe combat.txt no 50.0
```

### DLL Host (parser_host.exe)

```bash
# Basic usage (loads plugins from ./plugins/)
parser_host.exe <log_file>

# Example:
parser_host.exe combat.txt
```

---

## File Locations

After building, find your files here:

```
output/Release/
├── swtor_parser.exe                  # Direct integration
├── parser_host.exe                   # DLL host
├── swtor_parser_core.dll            # Core library
├── swtor_parser_core.lib            # Import library
└── example_damage_tracker_plugin.dll # Example plugin
```

---

## Getting Help

### Documentation

- **README.md** - Project overview
- **PLUGIN_DEVELOPMENT.md** - Plugin guide
- **API_REFERENCE.md** - API docs
- **BUILD_INSTRUCTIONS.md** - Build help
- **ARCHITECTURE.md** - System design

### Debugging

Enable debug output in your plugin:

```cpp
void ingest(parse_data_holder& data, CombatLine& line) override {
    #ifdef _DEBUG
    std::cout << "Processing: " << line.source.name << std::endl;
    #endif
    
    // Your code
}
```

Build in Debug mode for more info:
1. Select **Debug | x64**
2. Build
3. Run in debugger (F5)

---

## Tips & Tricks

### Fast Processing

For large logs, disable verbose output and timing:

```bash
swtor_parser.exe large_log.txt no 0.0
```

This processes at maximum speed.

### Find Sample Logs

SWTOR combat logs are typically located:

```
C:\Users\[YourName]\Documents\Star Wars - The Old Republic\CombatLogs\
```

### Test with Sample Data

The examples look for:
```
C:\Temp\Logs\combat_sample_dummy_log.txt
```

Create this directory and place a test log there for quick testing.

---

## What's Next?

Now that you have the parser running:

1. **Explore the examples** - See how built-in plugins work
2. **Create a simple plugin** - Start with a damage counter
3. **Read the docs** - Understand the full API
4. **Build your application** - Make something useful!

---

**Happy parsing!** 🚀

For questions or issues, check the full documentation or review the example code.
