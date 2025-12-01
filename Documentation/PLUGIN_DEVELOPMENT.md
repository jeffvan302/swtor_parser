# SWTOR Parser - Plugin Development Guide

Complete guide to developing plugins for the SWTOR Combat Parser, covering both built-in and external plugin development.

## Table of Contents

- [Introduction](#introduction)
- [Plugin Types](#plugin-types)
- [Plugin Architecture](#plugin-architecture)
- [Built-in Plugin Development](#built-in-plugin-development)
- [External Plugin Development](#external-plugin-development)
- [Plugin API Reference](#plugin-api-reference)
- [Best Practices](#best-practices)
- [Common Patterns](#common-patterns)
- [Debugging Plugins](#debugging-plugins)
- [Examples](#examples)

## Introduction

The SWTOR Parser plugin system allows you to extend the parser with custom functionality to process combat log data. Plugins receive parsed combat lines and can:

- Track statistics (DPS, HPS, damage taken, etc.)
- Monitor specific events or abilities
- Maintain entity-specific data
- Generate reports or visualizations
- Control parsing behavior (timing, filtering, etc.)

## Plugin Types

### 1. Built-in Plugins

**Definition**: Compiled directly into your executable alongside the core parser.

**Characteristics**:
- Full access to parser internals
- Inherits from `swtor::parse_plugin`
- Zero overhead for plugin interface
- Best performance
- Requires recompilation to update

**Use Cases**:
- Core functionality for your application
- Performance-critical operations
- Features requiring deep parser integration
- Single-developer projects

### 2. External Plugins (DLL)

**Definition**: Separate DLL modules loaded dynamically at runtime.

**Characteristics**:
- Inherits from `swtor::ExternalPluginBase`
- Loaded via plugin API
- Can be updated without recompiling host
- Slight overhead for DLL interface
- Enables third-party development

**Use Cases**:
- Optional features
- Third-party extensions
- Modular functionality
- Community-developed plugins

## Plugin Architecture

### Plugin Lifecycle

```
1. Creation
   ↓
2. Registration (set_id called)
   ↓
3. Processing (ingest called for each line)
   ↓
4. Reset (on combat end or manual reset)
   ↓
5. Destruction
```

### Execution Flow

```
Plugin Manager receives line
   ↓
Parse combat line
   ↓
Sort plugins by priority (ascending)
   ↓
Call ingest() for each plugin
   ↓
Update combat state
   ↓
Ready for next line
```

### Priority System

Plugins execute in priority order:
- **-1000 to -1**: Reserved for internal pre-processing
- **0**: Default priority
- **1 to 1000**: Application plugins
- **1001+**: Post-processing plugins

Lower numbers execute first.

## Built-in Plugin Development

### Step 1: Create Plugin Class

```cpp
// my_plugin.h
#pragma once
#include "../core/parse_manager.h"

namespace swtor {

class MyPlugin : public parse_plugin {
public:
    // Required: Return plugin name
    std::string name() const override;
    
    // Required: Process each combat line
    void ingest(parse_data_holder& parse_data, CombatLine& line) override;
    
    // Required: Reset plugin state
    void reset() override;
    
    // Optional: Set plugin ID (if you need to store it)
    void set_id(parse_data_holder& parse_data, uint16_t plugin_id) override;
    
    // Your custom public methods
    double get_dps() const;
    
private:
    // Your plugin state
    int64_t total_damage_ = 0;
    parse_data_holder* parse_data_ = nullptr;
    uint16_t id_ = 0;
};

} // namespace swtor
```

### Step 2: Implement Plugin

```cpp
// my_plugin.cpp
#include "my_plugin.h"

namespace swtor {

std::string MyPlugin::name() const {
    return "MyPlugin";
}

void MyPlugin::ingest(parse_data_holder& parse_data, CombatLine& line) {
    // Only process damage events
    if (line.event.type_id != KINDID_Event) {
        return;
    }
    
    if (line.event.action_id != static_cast<uint64_t>(EventActionType::Damage)) {
        return;
    }
    
    // Accumulate damage
    total_damage_ += line.tail.val.amount;
}

void MyPlugin::reset() {
    total_damage_ = 0;
}

void MyPlugin::set_id(parse_data_holder& parse_data, uint16_t plugin_id) {
    id_ = plugin_id;
    parse_data_ = &parse_data;
}

double MyPlugin::get_dps() const {
    if (!parse_data_ || !parse_data_->combat_state) {
        return 0.0;
    }
    
    int64_t combat_time = parse_data_->combat_state->get_combat_time();
    if (combat_time <= 0) {
        return 0.0;
    }
    
    return static_cast<double>(total_damage_) / (combat_time / 1000.0);
}

} // namespace swtor
```

### Step 3: Register Plugin

```cpp
// In your application code
#include "my_plugin.h"

int main() {
    swtor::plugin_manager manager;
    
    // Create and register plugin
    auto my_plugin = std::make_shared<swtor::MyPlugin>();
    my_plugin->set_priority(10);  // Optional: set priority
    manager.register_plugin(my_plugin);
    
    // Process lines
    for (const auto& line : log_lines) {
        manager.process_line(line);
    }
    
    // Access plugin results
    double dps = my_plugin->get_dps();
    std::cout << "DPS: " << dps << std::endl;
    
    return 0;
}
```

### Step 4: Add to Project

1. Add `my_plugin.h` and `my_plugin.cpp` to your Visual Studio project
2. Ensure they're included in the compilation
3. Build and run

## External Plugin Development

### Step 1: Create DLL Project

1. Create new Visual Studio project
2. Project type: **Dynamic Link Library (DLL)**
3. Name: `my_external_plugin`

### Step 2: Configure Project Settings

**C/C++ → General**:
- Additional Include Directories: `$(SolutionDir)core;$(ProjectDir)`
- C++ Language Standard: `C++20` or later

**C/C++ → Preprocessor**:
- Add: `PLUGIN_EXPORTS` (if needed)

**Linker → General**:
- Additional Library Directories: Path to `swtor_parser_core.lib`

**Linker → Input**:
- Additional Dependencies: `swtor_parser_core.lib`

### Step 3: Implement Plugin

```cpp
// my_external_plugin.cpp
#include "plugin_sdk.h"
#include <iostream>
#include <unordered_map>

class MyExternalPlugin : public swtor::ExternalPluginBase {
public:
    MyExternalPlugin() = default;
    ~MyExternalPlugin() override = default;
    
    std::string name() const override {
        return "MyExternalPlugin";
    }
    
    PluginInfo GetInfo() const override {
        return {
            "MyExternalPlugin",           // name
            "1.0.0",                      // version
            "Your Name",                  // author
            "My plugin description",      // description
            PLUGIN_API_VERSION            // API version (required)
        };
    }
    
    void ingest(swtor::parse_data_holder& parse_data, 
                swtor::CombatLine& line) override {
        // Your processing logic
        if (line.event.type_id == swtor::KINDID_Event) {
            if (line.event.action_id == 
                static_cast<uint64_t>(swtor::EventActionType::Damage)) {
                
                uint64_t source_id = line.source.id;
                damage_by_source_[source_id] += line.tail.val.amount;
            }
        }
    }
    
    void reset() override {
        damage_by_source_.clear();
        std::cout << "[MyExternalPlugin] Reset" << std::endl;
    }
    
    // Custom method - accessible via plugin instance
    int64_t get_damage_for(uint64_t source_id) const {
        auto it = damage_by_source_.find(source_id);
        return (it != damage_by_source_.end()) ? it->second : 0;
    }
    
private:
    std::unordered_map<uint64_t, int64_t> damage_by_source_;
};

// Export plugin - REQUIRED
EXPORT_PLUGIN(MyExternalPlugin)
```

### Step 4: Build and Deploy

1. Build project (Release x64 recommended)
2. Copy output DLL to `plugins` directory:
   ```
   output/Release/my_external_plugin.dll → [host_exe_dir]/plugins/
   ```
3. Run parser host - plugin will be auto-loaded

## Plugin API Reference

### Base Classes

#### `swtor::parse_plugin` (Built-in)

```cpp
class parse_plugin {
public:
    virtual ~parse_plugin() = default;
    
    // Required overrides
    virtual std::string name() const = 0;
    virtual void ingest(parse_data_holder& parse_data, CombatLine& line) = 0;
    virtual void reset() = 0;
    
    // Optional overrides
    virtual void set_id(parse_data_holder& parse_data, uint16_t plugin_id);
    
    // Plugin configuration
    void set_priority(int priority);
    int get_priority() const;
    
protected:
    uint16_t id = 0;  // Plugin ID assigned by manager
};
```

#### `swtor::ExternalPluginBase` (External DLL)

```cpp
class ExternalPluginBase : public parse_plugin {
public:
    // Additional required method
    virtual PluginInfo GetInfo() const = 0;
};

struct PluginInfo {
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    uint32_t api_version;  // Must be PLUGIN_API_VERSION
};
```

### Data Structures

#### `CombatLine`

```cpp
struct CombatLine {
    TimeStamp t;              // Timestamp information
    Entity source;            // Source entity (who)
    EffectAbilityThing event; // Event/ability (what)
    Entity target;            // Target entity (to whom)
    Tail tail;                // Value/result (how much)
    
    bool is_parsed;           // Parse success flag
};
```

#### `parse_data_holder`

```cpp
struct parse_data_holder {
    combat_state* combat_state;        // Combat state tracker
    entity_tracker* entities;          // Entity tracker
    CombatLine last_line;              // Most recent line
    CombatLine last_enter_combat;      // Line that started combat
    // ... other shared state
};
```

#### `combat_state`

```cpp
class combat_state {
public:
    bool is_in_combat() const;
    int64_t get_combat_time() const;
    std::string get_current_area() const;
    // ... more methods
};
```

### Event Types

```cpp
// Main type IDs
constexpr uint64_t KINDID_Event = 0;
constexpr uint64_t KINDID_Ability = 1;
constexpr uint64_t KINDID_Effect = 2;

// Event action types
enum class EventActionType : uint64_t {
    Damage = 836045448945500,
    Heal = 836045448945501,
    Death = 836045448945506,
    EnterCombat = 836045448945489,
    ExitCombat = 836045448945490,
    // ... more types
};
```

### Helper Methods

```cpp
// Check if line is a specific event type
bool is_damage_event(const CombatLine& line) {
    return line.event.type_id == KINDID_Event &&
           line.event.action_id == static_cast<uint64_t>(EventActionType::Damage);
}

// Access entity by ID
Entity* get_entity(parse_data_holder& data, uint64_t entity_id) {
    return data.entities->get_entity(entity_id);
}
```

## Best Practices

### Performance

1. **Minimize Processing**
   ```cpp
   void ingest(parse_data_holder& data, CombatLine& line) override {
       // Early return for irrelevant events
       if (line.event.type_id != KINDID_Event) return;
       if (line.event.action_id != my_target_action) return;
       
       // Only then do expensive processing
       process_event(data, line);
   }
   ```

2. **Cache Frequently Used Values**
   ```cpp
   // Bad: Recalculate every time
   double get_dps() const {
       return total_damage_ / (get_combat_time() / 1000.0);
   }
   
   // Good: Cache in ingest()
   void ingest(parse_data_holder& data, CombatLine& line) override {
       total_damage_ += damage;
       combat_time_ms_ = data.combat_state->get_combat_time();
       cached_dps_ = total_damage_ / (combat_time_ms_ / 1000.0);
   }
   ```

3. **Use Appropriate Data Structures**
   ```cpp
   // For frequent lookups by ID
   std::unordered_map<uint64_t, EntityData> entities_;
   
   // For sorted iteration
   std::map<uint64_t, EntityData> sorted_entities_;
   
   // For simple accumulation
   int64_t total_ = 0;
   ```

### Thread Safety

```cpp
// If plugin maintains shared state accessed from multiple threads
class ThreadSafePlugin : public parse_plugin {
private:
    mutable std::mutex mutex_;
    int64_t total_damage_ = 0;
    
public:
    void ingest(parse_data_holder& data, CombatLine& line) override {
        std::lock_guard<std::mutex> lock(mutex_);
        total_damage_ += line.tail.val.amount;
    }
    
    int64_t get_total() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return total_damage_;
    }
};
```

### Error Handling

```cpp
void ingest(parse_data_holder& data, CombatLine& line) override {
    try {
        // Your processing logic
        process_line(data, line);
    }
    catch (const std::exception& e) {
        std::cerr << "[" << name() << "] Error: " << e.what() << std::endl;
        // Don't let exceptions propagate to parser
    }
}
```

### State Management

```cpp
class StatefulPlugin : public parse_plugin {
private:
    // Clear state properly in reset()
    bool in_combat_ = false;
    std::vector<EventData> events_;
    std::unordered_map<uint64_t, Stats> stats_;
    
public:
    void reset() override {
        in_combat_ = false;
        events_.clear();
        stats_.clear();
        // Reset ALL plugin state
    }
};
```

## Common Patterns

### Pattern 1: Damage Tracker

```cpp
class DamageTracker : public parse_plugin {
private:
    struct DamageStats {
        int64_t total = 0;
        int64_t hits = 0;
        int64_t crits = 0;
    };
    
    std::unordered_map<uint64_t, DamageStats> damage_by_source_;
    
public:
    void ingest(parse_data_holder& data, CombatLine& line) override {
        if (!is_damage_event(line)) return;
        
        auto& stats = damage_by_source_[line.source.id];
        stats.total += line.tail.val.amount;
        stats.hits++;
        if (line.tail.val.is_crit) {
            stats.crits++;
        }
    }
    
    DamageStats get_stats_for(uint64_t entity_id) const {
        auto it = damage_by_source_.find(entity_id);
        return (it != damage_by_source_.end()) ? it->second : DamageStats{};
    }
};
```

### Pattern 2: Ability Cooldown Tracker

```cpp
class CooldownTracker : public parse_plugin {
private:
    struct AbilityUse {
        int64_t timestamp_ms;
        uint64_t ability_id;
    };
    
    std::vector<AbilityUse> ability_uses_;
    
public:
    void ingest(parse_data_holder& data, CombatLine& line) override {
        if (line.event.type_id == KINDID_Ability) {
            ability_uses_.push_back({
                line.t.refined_epoch_ms,
                line.event.id
            });
        }
    }
    
    int64_t get_cooldown_remaining(uint64_t ability_id, 
                                    int64_t current_time_ms) const {
        // Find last use
        for (auto it = ability_uses_.rbegin(); 
             it != ability_uses_.rend(); ++it) {
            if (it->ability_id == ability_id) {
                int64_t cooldown = get_ability_cooldown(ability_id);
                int64_t elapsed = current_time_ms - it->timestamp_ms;
                return std::max(0LL, cooldown - elapsed);
            }
        }
        return 0; // Not on cooldown
    }
};
```

### Pattern 3: Combat Session Tracker

```cpp
class SessionTracker : public parse_plugin {
private:
    struct CombatSession {
        int64_t start_time;
        int64_t end_time;
        int64_t duration;
        std::string area;
        int64_t total_damage;
    };
    
    std::vector<CombatSession> sessions_;
    CombatSession current_session_;
    bool in_combat_ = false;
    
public:
    void ingest(parse_data_holder& data, CombatLine& line) override {
        bool currently_in_combat = data.combat_state->is_in_combat();
        
        // Combat started
        if (currently_in_combat && !in_combat_) {
            current_session_ = CombatSession{};
            current_session_.start_time = line.t.refined_epoch_ms;
            current_session_.area = data.combat_state->get_current_area();
        }
        
        // Combat ended
        if (!currently_in_combat && in_combat_) {
            current_session_.end_time = line.t.refined_epoch_ms;
            current_session_.duration = 
                current_session_.end_time - current_session_.start_time;
            sessions_.push_back(current_session_);
        }
        
        // During combat: accumulate stats
        if (currently_in_combat && is_damage_event(line)) {
            current_session_.total_damage += line.tail.val.amount;
        }
        
        in_combat_ = currently_in_combat;
    }
    
    const std::vector<CombatSession>& get_sessions() const {
        return sessions_;
    }
};
```

## Debugging Plugins

### Enable Debug Output

```cpp
void ingest(parse_data_holder& data, CombatLine& line) override {
    #ifdef _DEBUG
    std::cout << "[" << name() << "] Processing line: " 
              << line.source.name << " -> " << line.target.name 
              << std::endl;
    #endif
    
    // Your logic
}
```

### Log to File

```cpp
class LoggingPlugin : public parse_plugin {
private:
    std::ofstream log_file_;
    
public:
    LoggingPlugin() {
        log_file_.open("plugin_debug.log");
    }
    
    void ingest(parse_data_holder& data, CombatLine& line) override {
        if (log_file_.is_open()) {
            log_file_ << line.t.print() << ": " 
                     << line.source.name << " -> " 
                     << line.event.name << std::endl;
        }
    }
};
```

### Validate Data

```cpp
void ingest(parse_data_holder& data, CombatLine& line) override {
    // Validate parse data
    if (!data.combat_state) {
        std::cerr << "[" << name() << "] ERROR: No combat state!" << std::endl;
        return;
    }
    
    if (!data.entities) {
        std::cerr << "[" << name() << "] ERROR: No entity tracker!" << std::endl;
        return;
    }
    
    // Validate line was parsed correctly
    if (!line.is_parsed) {
        std::cerr << "[" << name() << "] WARNING: Unparsed line" << std::endl;
        return;
    }
    
    // Your logic
}
```

## Examples

### Complete Built-in Plugin Example

See `example_swtor_parser/test_plugin.cpp` for a complete example of a built-in plugin that:
- Tracks damage and healing
- Calculates DPS/HPS
- Responds to combat state changes
- Properly manages state and cleanup

### Complete External Plugin Example

See `example_plugin/example_damage_tracker_plugin.cpp` for a complete example of an external DLL plugin that:
- Implements the external plugin API
- Tracks per-entity damage
- Handles combat state changes
- Properly exports plugin interface

## Advanced Topics

### Custom Plugin Configuration

```cpp
class ConfigurablePlugin : public parse_plugin {
private:
    bool enabled_ = true;
    int update_interval_ms_ = 1000;
    
public:
    void set_enabled(bool enabled) { enabled_ = enabled; }
    void set_update_interval(int ms) { update_interval_ms_ = ms; }
    
    void ingest(parse_data_holder& data, CombatLine& line) override {
        if (!enabled_) return;
        // Process with configuration
    }
};
```

### Inter-Plugin Communication

```cpp
// Define shared data structure
struct SharedPluginData {
    std::atomic<int64_t> total_damage{0};
    std::atomic<int64_t> total_healing{0};
};

// Plugin A writes
class ProducerPlugin : public parse_plugin {
private:
    SharedPluginData* shared_;
public:
    void set_shared_data(SharedPluginData* shared) { shared_ = shared; }
    void ingest(parse_data_holder& data, CombatLine& line) override {
        if (is_damage_event(line)) {
            shared_->total_damage += line.tail.val.amount;
        }
    }
};

// Plugin B reads
class ConsumerPlugin : public parse_plugin {
private:
    SharedPluginData* shared_;
public:
    void set_shared_data(SharedPluginData* shared) { shared_ = shared; }
    void report() {
        std::cout << "Total damage: " << shared_->total_damage << std::endl;
    }
};
```

## Troubleshooting

### Plugin Not Loaded

1. Check DLL is in `plugins` directory
2. Verify API version: `PLUGIN_API_VERSION`
3. Check `EXPORT_PLUGIN` macro is used
4. Ensure dependencies are met (swtor_parser_core.dll)
5. Check console for error messages

### Crashes

1. Check for null pointers before dereferencing
2. Validate `parse_data_holder` contents
3. Catch exceptions in `ingest()`
4. Verify memory management (no double-free)
5. Check thread safety if using multi-threading

### Unexpected Results

1. Verify event type filtering logic
2. Check reset() clears all state properly
3. Validate mathematical operations (division by zero, etc.)
4. Add debug logging to trace execution
5. Compare with reference plugin implementation

---

For more information, see:
- **API_REFERENCE.md** - Detailed API documentation
- **README.md** - Project overview
- **BUILD_INSTRUCTIONS.md** - Build setup
