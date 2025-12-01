# SWTOR Parser - API Reference

Complete reference for the SWTOR Combat Parser API, covering core classes, data structures, and plugin interfaces.

## Table of Contents

- [Core Classes](#core-classes)
- [Data Structures](#data-structures)
- [Plugin Interfaces](#plugin-interfaces)
- [Utility Functions](#utility-functions)
- [Constants and Enumerations](#constants-and-enumerations)
- [Error Handling](#error-handling)

## Core Classes

### plugin_manager

The main parser manager that coordinates parsing and plugin execution.

```cpp
namespace swtor {

class plugin_manager {
public:
    plugin_manager();
    ~plugin_manager();
    
    // Plugin Management
    void register_plugin(std::shared_ptr<parse_plugin> plugin);
    void remove_plugin(uint16_t plugin_id);
    void reset_all_plugins();
    void print_registered_plugins() const;
    
    // Line Processing
    void process_line(const std::string& raw_line);
    
    // State Queries
    bool is_in_combat() const;
    const CombatLine& get_last_line() const;
    const parse_data_holder& get_parse_data() const;
    const combat_state& get_combat_state() const;
    
    // Time Queries
    int64_t get_time_in_ms_epoch() const;
};

} // namespace swtor
```

#### Methods

##### register_plugin()
```cpp
void register_plugin(std::shared_ptr<parse_plugin> plugin);
```
Registers a plugin with the manager. Plugins are executed in priority order (lower priority values first).

**Parameters:**
- `plugin` - Shared pointer to plugin instance

**Example:**
```cpp
auto my_plugin = std::make_shared<MyPlugin>();
my_plugin->set_priority(10);
manager.register_plugin(my_plugin);
```

##### process_line()
```cpp
void process_line(const std::string& raw_line);
```
Parses a combat log line and passes it to all registered plugins.

**Parameters:**
- `raw_line` - Raw combat log line string

**Example:**
```cpp
std::string line = "[23:45:12.345] [@Player] [Ability {123}] [Event {456}]";
manager.process_line(line);
```

##### is_in_combat()
```cpp
bool is_in_combat() const;
```
Returns whether the parser is currently tracking active combat.

**Returns:** `true` if in combat, `false` otherwise

**Example:**
```cpp
if (manager.is_in_combat()) {
    int64_t combat_time = manager.get_combat_state().get_combat_time();
}
```

---

### combat_state

Tracks combat state, area transitions, and timing.

```cpp
namespace swtor {

class combat_state {
public:
    // Combat State
    bool is_in_combat() const;
    int64_t get_combat_time() const;  // Returns combat time in milliseconds
    
    // Area Tracking
    std::string get_current_area() const;
    AreaInfo get_last_area_entered() const;
    
    // Time Queries
    int64_t get_start_time() const;
    int64_t get_last_combat_time() const;
    
private:
    bool in_combat_;
    int64_t combat_start_time_;
    int64_t last_combat_time_;
    std::string current_area_;
};

} // namespace swtor
```

#### Methods

##### is_in_combat()
```cpp
bool is_in_combat() const;
```
Returns current combat state.

##### get_combat_time()
```cpp
int64_t get_combat_time() const;
```
Returns time elapsed since combat started, in milliseconds.

**Returns:** Combat duration in milliseconds, or 0 if not in combat

##### get_current_area()
```cpp
std::string get_current_area() const;
```
Returns the name of the current game area.

**Returns:** Area name string

---

### entity_tracker

Manages all entities (players, NPCs, companions) encountered in the log.

```cpp
namespace swtor {

class entity_tracker {
public:
    // Entity Access
    Entity* get_entity(uint64_t entity_id);
    Entity* owner();  // Returns player's entity
    
    // Entity Queries
    std::vector<Entity*> all_entities();
    std::vector<Entity*> all_players();
    std::vector<Entity*> all_npcs();
    
    // Entity Creation
    Entity* create_or_get_entity(uint64_t id, const std::string& name, bool is_player);
    
    // State Management
    void reset();
};

} // namespace swtor
```

#### Methods

##### get_entity()
```cpp
Entity* get_entity(uint64_t entity_id);
```
Retrieves an entity by ID.

**Parameters:**
- `entity_id` - Unique entity identifier

**Returns:** Pointer to Entity or nullptr if not found

##### owner()
```cpp
Entity* owner();
```
Returns the player's own entity.

**Returns:** Pointer to player Entity

##### all_entities()
```cpp
std::vector<Entity*> all_entities();
```
Returns all tracked entities.

**Returns:** Vector of Entity pointers

---

### NTPTimeKeeper

Provides NTP time synchronization and timezone support.

```cpp
namespace swtor {

class NTPTimeKeeper {
public:
    NTPTimeKeeper();
    
    // Synchronization
    bool synchronize();
    NTPResult getLastResult() const;
    
    // Time Queries
    int64_t getNTPTime() const;     // UTC time in milliseconds
    int64_t getLocalTime() const;   // Local time in milliseconds
    int64_t get_local_offset() const; // Timezone offset in milliseconds
    
    // Time Utilities
    int64_t getZeroHour(int64_t timestamp_ms) const;
    int64_t adjust_time(int64_t base_time, int64_t offset_ms) const;
    int64_t adjust_time(int64_t base_time, int days, int hours, 
                        int minutes, int seconds, int milliseconds) const;
};

struct NTPResult {
    std::string server;
    double offset_ms;
    double round_trip_ms;
    std::string error_message;
};

} // namespace swtor
```

#### Methods

##### synchronize()
```cpp
bool synchronize();
```
Synchronizes with NTP servers to get accurate time offset.

**Returns:** `true` if successful, `false` otherwise

**Example:**
```cpp
auto ntp = std::make_shared<swtor::NTPTimeKeeper>();
if (ntp->synchronize()) {
    auto result = ntp->getLastResult();
    std::cout << "Offset: " << result.offset_ms << " ms" << std::endl;
}
```

##### getNTPTime()
```cpp
int64_t getNTPTime() const;
```
Returns current UTC time in milliseconds since epoch.

##### getLocalTime()
```cpp
int64_t getLocalTime() const;
```
Returns current local time in milliseconds since epoch.

---

### TimeCruncher

Advanced timestamp processing with late-arrival adjustment.

```cpp
namespace swtor {

class TimeCruncher {
public:
    TimeCruncher(std::shared_ptr<NTPTimeKeeper> ntp_keeper, 
                 bool enable_late_arrival = true);
    
    // Timestamp Processing
    void process_timestamp(TimeStamp& timestamp);
    void adjust_for_late_arrival(TimeStamp& timestamp);
    
    // Configuration
    void set_late_arrival_threshold(int64_t threshold_ms);
    void enable_late_arrival_adjustment(bool enable);
};

} // namespace swtor
```

---

## Data Structures

### CombatLine

Represents a parsed combat log line.

```cpp
struct CombatLine {
    TimeStamp t;              // Timestamp information
    Entity source;            // Source entity (who)
    EffectAbilityThing event; // Event/ability (what)
    Entity target;            // Target entity (to whom)
    Tail tail;                // Value/result (how much)
    
    bool is_parsed;           // Parse success flag
    
    // Helper method
    std::string print() const;
};
```

#### Members

##### t (TimeStamp)
Timestamp information for when the event occurred.

##### source (Entity)
The entity that performed the action.

##### event (EffectAbilityThing)
The ability, effect, or event that occurred.

##### target (Entity)
The entity targeted by the action.

##### tail (Tail)
The result/value information (damage amount, etc.).

##### is_parsed
`true` if the line was successfully parsed, `false` otherwise.

---

### TimeStamp

Timestamp information with various formats.

```cpp
struct TimeStamp {
    int year;                    // Calendar year
    int month;                   // 1-12
    int day;                     // 1-31
    int h;                       // Hour (0-23)
    int m;                       // Minute (0-59)
    int s;                       // Second (0-59)
    int ms;                      // Milliseconds (0-999)
    
    int64_t combat_ms;           // Milliseconds since midnight
    int64_t refined_epoch_ms;    // Epoch timestamp (milliseconds)
    
    // Utilities
    std::string print() const;
    void update_combat_ms();
};
```

#### Methods

##### print()
```cpp
std::string print() const;
```
Returns formatted timestamp string.

**Returns:** String in format "YYYY-MM-DD HH:MM:SS.mmm"

##### update_combat_ms()
```cpp
void update_combat_ms();
```
Recalculates `combat_ms` from hour/minute/second/millisecond fields.

---

### Entity

Represents a game entity (player, NPC, companion).

```cpp
struct Entity {
    uint64_t id;                 // Unique entity ID
    std::string name;            // Entity name
    bool is_player;              // True if player/companion
    bool is_companion;           // True if companion
    bool is_dead;                // True if entity is dead
    
    // Stats
    HitPoints hp;                // Current health
    Entity target;               // Current target
    Entity* target_owner;        // Pointer to target's owner
    
    // Computed values
    double hitpoints_percent() const;
};

struct HitPoints {
    int64_t current;
    int64_t max;
    
    double percent() const {
        return max > 0 ? (100.0 * current / max) : 0.0;
    }
};
```

#### Methods

##### hitpoints_percent()
```cpp
double hitpoints_percent() const;
```
Returns current health as percentage.

**Returns:** Percentage (0.0 to 100.0)

---

### EffectAbilityThing

Represents an ability, effect, or event.

```cpp
struct EffectAbilityThing {
    uint64_t id;                 // Unique identifier
    uint64_t type_id;            // Kind ID (Event/Ability/Effect)
    uint64_t action_id;          // Action ID (for events)
    std::string name;            // Human-readable name
    
    // Utilities
    bool is_ability() const;
    bool is_effect() const;
    bool is_event() const;
};
```

#### Methods

##### is_ability()
```cpp
bool is_ability() const;
```
Returns `true` if this represents an ability.

##### is_effect()
```cpp
bool is_effect() const;
```
Returns `true` if this represents an effect.

##### is_event()
```cpp
bool is_event() const;
```
Returns `true` if this represents an event.

---

### Tail

Contains the value/result information from a combat line.

```cpp
struct Tail {
    Value val;                   // Primary value
    
    struct Value {
        int64_t amount;          // Damage/healing amount
        bool is_crit;            // Critical hit flag
        bool is_miss;            // Miss flag
        bool is_dodge;           // Dodge flag
        bool is_parry;           // Parry flag
        bool is_resist;          // Resist flag
        bool is_immune;          // Immune flag
        bool is_shield;          // Shield absorb flag
        int shield_amount;       // Shield amount absorbed
    };
};
```

---

### parse_data_holder

Shared data structure passed to all plugins.

```cpp
struct parse_data_holder {
    combat_state* combat_state;        // Combat state tracker
    entity_tracker* entities;          // Entity tracker
    
    CombatLine last_line;              // Most recent parsed line
    CombatLine last_enter_combat;      // Line that started combat
    
    std::shared_ptr<NTPTimeKeeper> ntp_keeper;
    std::shared_ptr<TimeCruncher> time_cruncher;
};
```

#### Members

##### combat_state
Pointer to combat state tracker. Use to query combat status, timing, and area.

##### entities
Pointer to entity tracker. Use to access entity information.

##### last_line
The most recently processed CombatLine.

##### last_enter_combat
The CombatLine that triggered entering combat.

---

## Plugin Interfaces

### parse_plugin (Built-in)

Base class for plugins compiled into the executable.

```cpp
namespace swtor {

class parse_plugin {
public:
    virtual ~parse_plugin() = default;
    
    // Required overrides
    virtual std::string name() const = 0;
    virtual void ingest(parse_data_holder& parse_data, CombatLine& line) = 0;
    virtual void reset() = 0;
    
    // Optional override
    virtual void set_id(parse_data_holder& parse_data, uint16_t plugin_id) {
        id = plugin_id;
    }
    
    // Configuration
    void set_priority(int priority);
    int get_priority() const;
    
protected:
    uint16_t id = 0;
    int priority_ = 0;
};

} // namespace swtor
```

#### Virtual Methods

##### name()
```cpp
virtual std::string name() const = 0;
```
Returns the plugin's name.

**Must be overridden.**

##### ingest()
```cpp
virtual void ingest(parse_data_holder& parse_data, CombatLine& line) = 0;
```
Called for each parsed combat line.

**Parameters:**
- `parse_data` - Shared parse data and state
- `line` - Parsed combat line to process

**Must be overridden.**

##### reset()
```cpp
virtual void reset() = 0;
```
Called when plugin state should be reset (combat end, manual reset).

**Must be overridden.**

##### set_id()
```cpp
virtual void set_id(parse_data_holder& parse_data, uint16_t plugin_id);
```
Called when plugin is registered to assign it a unique ID.

**Optional override.**

#### Configuration Methods

##### set_priority()
```cpp
void set_priority(int priority);
```
Sets plugin execution priority. Lower values execute first.

**Parameters:**
- `priority` - Priority value (default: 0)

##### get_priority()
```cpp
int get_priority() const;
```
Returns current priority value.

---

### ExternalPluginBase (External DLL)

Base class for external DLL plugins.

```cpp
namespace swtor {

class ExternalPluginBase : public parse_plugin {
public:
    virtual ~ExternalPluginBase() = default;
    
    // Additional required method
    virtual PluginInfo GetInfo() const = 0;
};

struct PluginInfo {
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    uint32_t api_version;  // Must equal PLUGIN_API_VERSION
};

} // namespace swtor
```

#### Virtual Methods

##### GetInfo()
```cpp
virtual PluginInfo GetInfo() const = 0;
```
Returns plugin metadata.

**Must be overridden.**

**Example:**
```cpp
PluginInfo GetInfo() const override {
    return {
        "MyPlugin",           // name
        "1.0.0",             // version
        "John Doe",          // author
        "Does cool things",  // description
        PLUGIN_API_VERSION   // api_version (required!)
    };
}
```

---

### Plugin Export Macro

For external DLL plugins, use this macro to export the plugin:

```cpp
EXPORT_PLUGIN(PluginClassName)
```

**Example:**
```cpp
class MyExternalPlugin : public swtor::ExternalPluginBase {
    // ... implementation
};

EXPORT_PLUGIN(MyExternalPlugin)
```

This macro expands to the necessary C functions for plugin loading.

---

## Utility Functions

### parse_combat_line()

Parses a raw combat log line string.

```cpp
namespace swtor {

ParseStatus parse_combat_line(const std::string& raw_line, CombatLine& out_line);

enum class ParseStatus {
    Success,
    InvalidFormat,
    InvalidTimestamp,
    MissingComponents,
    UnknownError
};

} // namespace swtor
```

**Parameters:**
- `raw_line` - Raw log line string
- `out_line` - Output CombatLine structure

**Returns:** ParseStatus indicating success or failure reason

**Example:**
```cpp
std::string line = "[23:45:12.345] [@Player] [Ability {123}] [Event {456}]";
swtor::CombatLine parsed;

if (swtor::parse_combat_line(line, parsed) == swtor::ParseStatus::Success) {
    std::cout << "Parsed: " << parsed.source.name << std::endl;
}
```

---

### formatDurationMs()

Formats a duration in milliseconds as a readable string.

```cpp
namespace swtor {

std::string formatDurationMs(int64_t duration_ms);

} // namespace swtor
```

**Parameters:**
- `duration_ms` - Duration in milliseconds

**Returns:** Formatted string (e.g., "2m 35s", "1h 23m 45s")

**Example:**
```cpp
int64_t combat_time = 155000;  // 2 minutes 35 seconds
std::string formatted = swtor::formatDurationMs(combat_time);
// Result: "2m 35s"
```

---

## Constants and Enumerations

### Event Type IDs

```cpp
namespace swtor {

// Main kind IDs
constexpr uint64_t KINDID_Event = 0;
constexpr uint64_t KINDID_Ability = 1;
constexpr uint64_t KINDID_Effect = 2;

} // namespace swtor
```

### Event Action Types

```cpp
namespace swtor {

enum class EventActionType : uint64_t {
    // Combat Events
    Damage = 836045448945500,
    Heal = 836045448945501,
    Death = 836045448945506,
    
    // Combat State
    EnterCombat = 836045448945489,
    ExitCombat = 836045448945490,
    
    // Threat
    Threat = 836045448945507,
    
    // Area Changes
    AreaEntered = 836045448945488,
    
    // Add more as discovered
};

} // namespace swtor
```

### Plugin API Version

```cpp
#define PLUGIN_API_VERSION 1
```

Current plugin API version. External plugins must return this value in `GetInfo().api_version`.

---

## Error Handling

### Exception Safety

The parser is designed to be exception-safe:

```cpp
// Plugin ingest() should catch exceptions
void ingest(parse_data_holder& data, CombatLine& line) override {
    try {
        // Your processing
    }
    catch (const std::exception& e) {
        std::cerr << "[Plugin] Error: " << e.what() << std::endl;
        // Don't propagate - let other plugins continue
    }
}
```

### Null Pointer Checks

Always validate pointers from parse_data_holder:

```cpp
void ingest(parse_data_holder& data, CombatLine& line) override {
    if (!data.combat_state) {
        // Handle error
        return;
    }
    
    if (!data.entities) {
        // Handle error
        return;
    }
    
    // Safe to use
    bool in_combat = data.combat_state->is_in_combat();
}
```

### Parse Status Checks

Check if line was successfully parsed:

```cpp
void ingest(parse_data_holder& data, CombatLine& line) override {
    if (!line.is_parsed) {
        // Skip unparsed lines
        return;
    }
    
    // Process parsed line
}
```

---

## Usage Examples

### Example 1: Simple DPS Tracker

```cpp
class DPSTracker : public swtor::parse_plugin {
private:
    int64_t total_damage_ = 0;
    swtor::parse_data_holder* data_ = nullptr;
    
public:
    std::string name() const override {
        return "DPSTracker";
    }
    
    void ingest(swtor::parse_data_holder& data, swtor::CombatLine& line) override {
        if (!line.is_parsed) return;
        
        // Only track damage events
        if (line.event.type_id != swtor::KINDID_Event) return;
        if (line.event.action_id != 
            static_cast<uint64_t>(swtor::EventActionType::Damage)) return;
        
        total_damage_ += line.tail.val.amount;
    }
    
    void set_id(swtor::parse_data_holder& data, uint16_t plugin_id) override {
        id = plugin_id;
        data_ = &data;
    }
    
    void reset() override {
        total_damage_ = 0;
    }
    
    double get_dps() const {
        if (!data_ || !data_->combat_state) return 0.0;
        
        int64_t time_ms = data_->combat_state->get_combat_time();
        if (time_ms <= 0) return 0.0;
        
        return static_cast<double>(total_damage_) / (time_ms / 1000.0);
    }
};
```

### Example 2: Entity Health Tracker

```cpp
class HealthTracker : public swtor::parse_plugin {
private:
    struct HealthSnapshot {
        int64_t timestamp;
        int64_t current_hp;
        int64_t max_hp;
    };
    
    std::unordered_map<uint64_t, std::vector<HealthSnapshot>> health_history_;
    
public:
    std::string name() const override {
        return "HealthTracker";
    }
    
    void ingest(swtor::parse_data_holder& data, swtor::CombatLine& line) override {
        if (!data.entities) return;
        
        // Track source entity health
        if (auto* entity = data.entities->get_entity(line.source.id)) {
            health_history_[line.source.id].push_back({
                line.t.refined_epoch_ms,
                entity->hp.current,
                entity->hp.max
            });
        }
        
        // Track target entity health
        if (auto* entity = data.entities->get_entity(line.target.id)) {
            health_history_[line.target.id].push_back({
                line.t.refined_epoch_ms,
                entity->hp.current,
                entity->hp.max
            });
        }
    }
    
    void reset() override {
        health_history_.clear();
    }
    
    const std::vector<HealthSnapshot>& get_history(uint64_t entity_id) const {
        static std::vector<HealthSnapshot> empty;
        auto it = health_history_.find(entity_id);
        return (it != health_history_.end()) ? it->second : empty;
    }
};
```

---

## Thread Safety Notes

The parser itself is **not thread-safe**. Do not call `process_line()` from multiple threads simultaneously.

However, plugins can use internal synchronization if they access shared state from multiple threads (e.g., for UI updates).

```cpp
class ThreadSafePlugin : public swtor::parse_plugin {
private:
    mutable std::mutex mutex_;
    int64_t total_ = 0;
    
public:
    void ingest(swtor::parse_data_holder& data, swtor::CombatLine& line) override {
        std::lock_guard<std::mutex> lock(mutex_);
        total_ += line.tail.val.amount;
    }
    
    int64_t get_total() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return total_;
    }
};
```

---

## Performance Considerations

### Minimize Work in ingest()

The `ingest()` method is called for every log line. Keep it fast:

```cpp
// BAD: Expensive string operations
void ingest(swtor::parse_data_holder& data, swtor::CombatLine& line) override {
    std::string msg = "Processing: " + line.source.name + " -> " + line.target.name;
    std::cout << msg << std::endl;  // I/O is slow!
}

// GOOD: Early return and minimal processing
void ingest(swtor::parse_data_holder& data, swtor::CombatLine& line) override {
    if (line.event.type_id != KINDID_Event) return;
    total_damage_ += line.tail.val.amount;  // Fast integer operation
}
```

### Cache Frequently Accessed Data

```cpp
// BAD: Recalculate every time
double get_dps() const {
    int64_t time = data_->combat_state->get_combat_time();
    return total_ / (time / 1000.0);
}

// GOOD: Cache in ingest()
void ingest(swtor::parse_data_holder& data, swtor::CombatLine& line) override {
    total_ += line.tail.val.amount;
    combat_time_ = data.combat_state->get_combat_time();
    cached_dps_ = total_ / (combat_time_ / 1000.0);  // Cache result
}

double get_dps() const {
    return cached_dps_;  // Return cached value
}
```

---

For more information:
- **README.md** - Project overview
- **PLUGIN_DEVELOPMENT.md** - Plugin development guide
- **BUILD_INSTRUCTIONS.md** - Build setup
