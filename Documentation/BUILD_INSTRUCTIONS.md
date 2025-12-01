# SWTOR Parser - Build Instructions

Comprehensive guide to building the SWTOR Combat Parser project in various configurations.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Quick Start](#quick-start)
- [Build Configurations](#build-configurations)
- [Building Direct Integration](#building-direct-integration)
- [Building DLL Architecture](#building-dll-architecture)
- [Building External Plugins](#building-external-plugins)
- [Project Configuration Details](#project-configuration-details)
- [Troubleshooting](#troubleshooting)
- [Advanced Build Options](#advanced-build-options)

## Prerequisites

### Required Software

1. **Visual Studio 2022**
   - Version 17.0 or later
   - Platform Toolset: v143
   - Download: [Visual Studio 2022](https://visualstudio.microsoft.com/vs/)

2. **Windows SDK**
   - Version 10.0 or later
   - Included with Visual Studio installer

3. **C++ Desktop Development Workload**
   - Required components:
     - MSVC v143 - VS 2022 C++ x64/x86 build tools
     - Windows 10/11 SDK
     - C++ CMake tools (optional, for CMake builds)

### System Requirements

- **Operating System**: Windows 10 (1809 or later) or Windows 11
- **Architecture**: x64 (64-bit)
- **Memory**: 8GB RAM minimum (16GB recommended for large builds)
- **Disk Space**: 500MB for source + 2GB for build outputs

### C++ Standard

- **C++20** is required
- Features used:
  - `std::filesystem`
  - `std::span`
  - `std::string_view`
  - Concepts (in some headers)
  - Designated initializers

## Quick Start

### Option 1: Direct Integration (Single Executable)

```bash
# 1. Open solution
start swtor_parser.sln

# 2. Select configuration: Release | x64

# 3. Build -> Build Solution (Ctrl+Shift+B)

# 4. Output at: output/Release/swtor_parser.exe
```

### Option 2: DLL Architecture (Core + Plugins)

```bash
# 1. Open solution
start swtor_parser_dll_hosting.sln

# 2. Select configuration: Release | x64

# 3. Build -> Build Solution (Ctrl+Shift+B)

# 4. Outputs at:
#    - output/Release/swtor_parser_core.dll
#    - output/Release/swtor_parser_core.lib
#    - output/Release/parser_host.exe
#    - output/Release/example_damage_tracker_plugin.dll
```

## Build Configurations

### Available Configurations

| Configuration | Platform | Purpose | Output |
|---------------|----------|---------|--------|
| Debug | x64 | Development, debugging | Unoptimized, with debug symbols |
| Release | x64 | Production use | Optimized, minimal debug info |

### Configuration Details

#### Debug Configuration

**Purpose**: Development and debugging

**Characteristics**:
- Optimizations disabled (`/Od`)
- Debug symbols included (`/Zi`)
- Runtime checks enabled (`/RTC1`, `/sdl`)
- Defines: `_DEBUG`, `_CONSOLE`

**Performance**: ~5-10x slower than Release

**Use when**:
- Debugging issues
- Developing new features
- Learning the codebase

#### Release Configuration

**Purpose**: Production deployment

**Characteristics**:
- Full optimizations (`/O2`)
- Intrinsic functions (`/Oi`)
- Whole program optimization (`/GL`)
- Link-time code generation (`/LTCG`)
- Defines: `NDEBUG`, `_CONSOLE`

**Performance**: Maximum throughput

**Use when**:
- Benchmarking
- Processing large log files
- Production deployment

## Building Direct Integration

The direct integration approach compiles the core parser directly into your executable.

### Project: `swtor_parser.vcxproj`

**Location**: `example_swtor_parser/swtor_parser.vcxproj`

**Solution**: `swtor_parser.sln`

### Step-by-Step Build

#### 1. Open Solution

```bash
# Using File Explorer
Double-click swtor_parser.sln

# Or from command line
start swtor_parser.sln

# Or from Developer Command Prompt
devenv swtor_parser.sln
```

#### 2. Verify Project Configuration

**Project Properties** (Right-click project → Properties):

**General**:
- Configuration Type: `Application (.exe)`
- Platform Toolset: `Visual Studio 2022 (v143)`
- Windows SDK Version: `10.0` (latest installed)
- C++ Language Standard: `ISO C++20 Standard (/std:c++20)`

**C/C++ → General**:
- Additional Include Directories: `$(ProjectDir)..\core`

**C/C++ → Preprocessor**:
- Preprocessor Definitions:
  - Debug: `_DEBUG;_CONSOLE;%(PreprocessorDefinitions)`
  - Release: `NDEBUG;_CONSOLE;%(PreprocessorDefinitions)`

**Linker → General**:
- Output File: `$(OutDir)swtor_parser.exe`

#### 3. Build

```bash
# In Visual Studio
Build → Build Solution (Ctrl+Shift+B)

# Or from Developer Command Prompt
msbuild swtor_parser.sln /p:Configuration=Release /p:Platform=x64
```

#### 4. Verify Output

```bash
# Check for output file
dir output\Release\swtor_parser.exe

# Test run
cd output\Release
swtor_parser.exe --help
```

### Files Included in Build

The project compiles these core files directly:

**Core Parser Files** (from `../core/`):
- `swtor_parser.cpp` - Main parser implementation
- `parse_manager.cpp` - Plugin manager
- `combat_state.cpp` - Combat state tracker
- `ntp_timekeeper_swtor.cpp` - NTP time sync
- `time_cruncher_swtor.cpp` - Timestamp processing
- `timing_plugin.cpp` - Built-in timing plugin

**Application Files**:
- `example_swtor_integration.cpp` - Main application
- `test_plugin.cpp` - Example plugin

**Note**: Missing `core/` directory? See [Troubleshooting](#missing-core-directory).

## Building DLL Architecture

The DLL architecture builds separate components that can be loaded dynamically.

### Solution: `swtor_parser_dll_hosting.sln`

**Components**:
1. `swtor_parser_core` - Core DLL
2. `parser_host` - Host executable
3. `example_damage_tracker_plugin` - Example external plugin

### Build Order

The solution is configured with proper dependencies:
```
swtor_parser_core (builds first)
    ↓
example_damage_tracker_plugin (depends on core)
    ↓
parser_host (depends on plugin)
```

### 1. Building Core DLL

**Project**: `core/swtor_parser_core.vcxproj`

#### Configuration

**General**:
- Configuration Type: `Dynamic Library (.dll)`
- Platform Toolset: `Visual Studio 2022 (v143)`
- C++ Language Standard: `ISO C++20 Standard`

**C/C++ → Preprocessor**:
- Additional Definitions: `SWTOR_PARSER_EXPORTS` (for DLL exports)

**Linker → General**:
- Output File: `$(OutDir)swtor_parser_core.dll`
- Import Library: `$(OutDir)swtor_parser_core.lib`

#### Build

```bash
# Build just the core
msbuild core\swtor_parser_core.vcxproj /p:Configuration=Release /p:Platform=x64
```

#### Outputs

- `swtor_parser_core.dll` - Runtime library
- `swtor_parser_core.lib` - Import library (for linking plugins)
- `swtor_parser_core.pdb` - Debug symbols (Debug config only)

### 2. Building Parser Host

**Project**: `host_parsing/parser_host.vcxproj`

#### Configuration

**General**:
- Configuration Type: `Application (.exe)`

**C/C++ → General**:
- Additional Include Directories: `$(ProjectDir)..\core`

**Linker → Input**:
- Additional Dependencies: `swtor_parser_core.lib` (automatic via dependency)

#### Build

```bash
# Build just the host
msbuild host_parsing\parser_host.vcxproj /p:Configuration=Release /p:Platform=x64
```

#### Output

- `parser_host.exe` - Host application that loads core DLL and plugins

### 3. Building External Plugins

**Project**: `example_plugin/example_damage_tracker_plugin.vcxproj`

#### Configuration

**General**:
- Configuration Type: `Dynamic Library (.dll)`

**C/C++ → General**:
- Additional Include Directories: `$(ProjectDir)..\core`

**Linker → Input**:
- Additional Dependencies: `swtor_parser_core.lib`

**Linker → General**:
- Additional Library Directories: `$(OutDir)` (to find swtor_parser_core.lib)

#### Build

```bash
# Build just the plugin
msbuild example_plugin\example_damage_tracker_plugin.vcxproj /p:Configuration=Release /p:Platform=x64
```

#### Output

- `example_damage_tracker_plugin.dll` - External plugin

### 4. Complete Build

Build all components at once:

```bash
# Using Visual Studio
Build → Build Solution

# Or from command line
msbuild swtor_parser_dll_hosting.sln /p:Configuration=Release /p:Platform=x64
```

### 5. Deployment

After building, organize files for deployment:

```
deployment/
├── parser_host.exe                          # Host application
├── swtor_parser_core.dll                    # Core library
└── plugins/                                  # Plugin directory
    └── example_damage_tracker_plugin.dll    # External plugins
```

```bash
# Create deployment structure
mkdir deployment
mkdir deployment\plugins

# Copy files
copy output\Release\parser_host.exe deployment\
copy output\Release\swtor_parser_core.dll deployment\
copy output\Release\example_damage_tracker_plugin.dll deployment\plugins\
```

## Building External Plugins

### Creating a New Plugin Project

#### 1. Create New Project

In Visual Studio:
1. File → New → Project
2. Select "Dynamic-Link Library (DLL)"
3. Name: `my_plugin`
4. Location: Next to existing projects

#### 2. Configure Project

**General**:
```
Configuration Type: Dynamic Library (.dll)
Platform Toolset: Visual Studio 2022 (v143)
C++ Language Standard: ISO C++20 Standard (/std:c++20)
```

**C/C++ → General**:
```
Additional Include Directories: ..\core;%(AdditionalIncludeDirectories)
```

**C/C++ → Preprocessor**:
```
Preprocessor Definitions: (none required)
```

**Linker → General**:
```
Additional Library Directories: $(SolutionDir)output\$(Configuration)
```

**Linker → Input**:
```
Additional Dependencies: swtor_parser_core.lib;%(AdditionalDependencies)
```

#### 3. Implement Plugin

```cpp
// my_plugin.cpp
#include "plugin_sdk.h"

class MyPlugin : public swtor::ExternalPluginBase {
public:
    std::string name() const override { return "MyPlugin"; }
    
    PluginInfo GetInfo() const override {
        return {"MyPlugin", "1.0.0", "Me", "Description", PLUGIN_API_VERSION};
    }
    
    void ingest(swtor::parse_data_holder& data, swtor::CombatLine& line) override {
        // Your logic
    }
    
    void reset() override {
        // Reset state
    }
};

EXPORT_PLUGIN(MyPlugin)
```

#### 4. Build

```bash
# Build your plugin
msbuild my_plugin.vcxproj /p:Configuration=Release /p:Platform=x64
```

#### 5. Deploy

```bash
# Copy to plugins directory
copy output\Release\my_plugin.dll deployment\plugins\
```

## Project Configuration Details

### Common Settings

All projects should use:

**C/C++ → General**:
- Warning Level: `Level3` (`/W3`)
- SDL checks: `Yes` (`/sdl`)
- Conformance mode: `Yes` (`/permissive-`)

**C/C++ → Language**:
- C++ Language Standard: `ISO C++20 Standard` (`/std:c++20`)
- Enable Run-Time Type Information: `Yes` (`/GR`)

**C/C++ → Code Generation**:
- Runtime Library: 
  - Debug: `Multi-threaded Debug DLL` (`/MDd`)
  - Release: `Multi-threaded DLL` (`/MD`)

### Output Directories

Configured to use common output paths:

```
$(SolutionDir)output\$(Configuration)\
```

This creates:
```
solution_root/
└── output/
    ├── Debug/
    │   ├── *.exe
    │   ├── *.dll
    │   └── *.lib
    └── Release/
        ├── *.exe
        ├── *.dll
        └── *.lib
```

### Intermediate Directories

Build artifacts go to:

```
$(SolutionDir)obj\$(Configuration)\$(ProjectName)\
```

## Troubleshooting

### Missing core/ Directory

**Problem**: Build errors about missing core files.

**Solution**: The core library source files are not in the uploaded archive. You need:

```
core/
├── swtor_parser.cpp/.h
├── parse_manager.cpp/.h
├── combat_state.cpp/.h
├── ntp_timekeeper_swtor.cpp/.h
├── time_cruncher_swtor.cpp/.h
├── timing_plugin.cpp/.h
├── parse_plugin.h
├── plugin_api.h
├── base_classes.h
├── stat_keeper.h
└── memory_helper.h
```

**Workaround**: Contact the project maintainer for the complete core library source.

### Cannot Find swtor_parser_core.lib

**Problem**: Linker error when building plugins or host.

**Solution**: 
1. Build `swtor_parser_core` project first
2. Verify `swtor_parser_core.lib` exists in output directory
3. Check linker Additional Library Directories includes output path

### DLL Not Found at Runtime

**Problem**: Host cannot find `swtor_parser_core.dll`.

**Solution**:
1. Copy `swtor_parser_core.dll` to same directory as `parser_host.exe`
2. Or add DLL directory to PATH
3. Or use `SetDllDirectory()` in code

### Plugin Not Loading

**Problem**: External plugin DLL not loading.

**Solution**:
1. Verify plugin DLL is in `plugins` subdirectory
2. Check API version matches: `PLUGIN_API_VERSION`
3. Verify `EXPORT_PLUGIN` macro is used
4. Check console output for error messages
5. Ensure `swtor_parser_core.dll` is accessible

### LNK4098: Defaultlib Conflict

**Problem**: Link warning about conflicting runtime libraries.

**Solution**: Ensure all projects use the same runtime library:
- Debug: `/MDd`
- Release: `/MD`

Check in Project Properties → C/C++ → Code Generation → Runtime Library

### C2039: 'filesystem' is not a member of 'std'

**Problem**: Compiler doesn't recognize C++20 features.

**Solution**: Verify C++ Language Standard is set to C++20:
1. Right-click project → Properties
2. C/C++ → Language
3. C++ Language Standard: `ISO C++20 Standard (/std:c++20)`

### Incremental Linking Errors

**Problem**: LNK1000 or similar errors during incremental linking.

**Solution**:
```bash
# Clean and rebuild
Build → Clean Solution
Build → Rebuild Solution

# Or delete intermediate files manually
rd /s /q obj
rd /s /q output
```

## Advanced Build Options

### Building from Command Line

#### Using MSBuild

```bash
# Open Developer Command Prompt for VS 2022
# Then:

# Build specific configuration
msbuild swtor_parser.sln /p:Configuration=Release /p:Platform=x64

# Clean before build
msbuild swtor_parser.sln /t:Clean /p:Configuration=Release /p:Platform=x64
msbuild swtor_parser.sln /t:Build /p:Configuration=Release /p:Platform=x64

# Parallel build (faster)
msbuild swtor_parser.sln /p:Configuration=Release /p:Platform=x64 /m

# Verbose output
msbuild swtor_parser.sln /p:Configuration=Release /p:Platform=x64 /v:detailed

# Build specific project
msbuild example_swtor_parser\swtor_parser.vcxproj /p:Configuration=Release /p:Platform=x64
```

#### Using devenv

```bash
# Build solution
devenv swtor_parser.sln /build "Release|x64"

# Rebuild solution
devenv swtor_parser.sln /rebuild "Release|x64"

# Clean solution
devenv swtor_parser.sln /clean "Release|x64"

# Build specific project
devenv swtor_parser.sln /build "Release|x64" /project swtor_parser
```

### Build Automation Scripts

#### Batch Script

```batch
@echo off
REM build_all.bat

echo Building SWTOR Parser (Release x64)...

REM Clean
msbuild swtor_parser_dll_hosting.sln /t:Clean /p:Configuration=Release /p:Platform=x64 /v:quiet
if errorlevel 1 goto error

REM Build
msbuild swtor_parser_dll_hosting.sln /t:Build /p:Configuration=Release /p:Platform=x64 /v:minimal /m
if errorlevel 1 goto error

echo.
echo Build successful!
echo Output: output\Release\
goto end

:error
echo.
echo Build failed!
exit /b 1

:end
```

#### PowerShell Script

```powershell
# build_all.ps1

$ErrorActionPreference = "Stop"

Write-Host "Building SWTOR Parser (Release x64)..." -ForegroundColor Cyan

# Clean
Write-Host "Cleaning..." -ForegroundColor Yellow
& msbuild swtor_parser_dll_hosting.sln /t:Clean /p:Configuration=Release /p:Platform=x64 /v:quiet
if ($LASTEXITCODE -ne 0) { throw "Clean failed" }

# Build
Write-Host "Building..." -ForegroundColor Yellow
& msbuild swtor_parser_dll_hosting.sln /t:Build /p:Configuration=Release /p:Platform=x64 /v:minimal /m
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

Write-Host "`nBuild successful!" -ForegroundColor Green
Write-Host "Output: output\Release\" -ForegroundColor Cyan
```

### Custom Build Configurations

To create a custom configuration (e.g., "Profile"):

1. Configuration Manager → Active Solution Configuration → New
2. Name: `Profile`
3. Copy settings from: `Release`
4. Modify settings:
   - Enable optimizations
   - Include debug symbols
   - Add profiling instrumentation

### Build Performance Tips

1. **Parallel Build**: Use `/m` flag
   ```bash
   msbuild solution.sln /m
   ```

2. **Incremental Linking**: Enabled by default in Debug
   - Faster linking for small changes
   - Disable for Release: `/INCREMENTAL:NO`

3. **Precompiled Headers**: Consider adding for large projects
   ```cpp
   // pch.h
   #pragma once
   #include <vector>
   #include <memory>
   #include <unordered_map>
   // ... other frequently-used headers
   ```

4. **Build Cache**: Enable in Tools → Options → Projects and Solutions → Build and Run
   - "Maximum number of parallel project builds": 8 (adjust for your CPU)

## Continuous Integration

### GitHub Actions Example

```yaml
name: Build

on: [push, pull_request]

jobs:
  build:
    runs-on: windows-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Setup MSBuild
      uses: microsoft/setup-msbuild@v1
    
    - name: Build
      run: msbuild swtor_parser_dll_hosting.sln /p:Configuration=Release /p:Platform=x64 /m
    
    - name: Upload artifacts
      uses: actions/upload-artifact@v3
      with:
        name: swtor-parser
        path: output/Release/
```

## Additional Resources

- [MSBuild Command-Line Reference](https://docs.microsoft.com/en-us/visualstudio/msbuild/msbuild-command-line-reference)
- [Visual Studio Build Tools](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022)
- [C++20 Features](https://en.cppreference.com/w/cpp/20)

---

For more information:
- **README.md** - Project overview
- **PLUGIN_DEVELOPMENT.md** - Plugin development guide
- **API_REFERENCE.md** - API documentation
