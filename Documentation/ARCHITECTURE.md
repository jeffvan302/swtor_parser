# SWTOR Parser - Architecture Overview

Detailed explanation of the SWTOR Combat Parser architecture, design decisions, and component interactions.

## Table of Contents

- [System Overview](#system-overview)
- [Component Architecture](#component-architecture)
- [Data Flow](#data-flow)
- [Plugin System Design](#plugin-system-design)
- [Time Synchronization](#time-synchronization)
- [State Management](#state-management)
- [Design Decisions](#design-decisions)
- [Performance Considerations](#performance-considerations)

## System Overview

The SWTOR Parser is designed as a modular, extensible system for parsing Star Wars: The Old Republic combat logs. The architecture supports two deployment models:

1. **Monolithic** - Core parser compiled directly into application
2. **Modular** - Core as DLL with dynamically loadable plugins

### High-Level Architecture

```
┌─────────────────────────────────────────────────┐
│          Application Layer                       │
│  ┌───────────────┐      ┌───────────────────┐  │
│  │ Direct App    │  OR  │ DLL Host App      │  │
│  │ (Monolithic)  │      │ (Modular)         │  │
│  └───────┬───────┘      └─────────┬─────────┘  │
└──────────┼──────────────────────────┼───────────┘
           │                          │
┌──────────┼──────────────────────────┼───────────┐
│          │     Core Parser Layer    │           │
│  ┌───────▼──────────────────────────▼───────┐  │
│  │        Plugin Manager                    │  │
│  │  ┌────────────────────────────────────┐  │  │
│  │  │  Parse Pipeline                    │  │  │
│  │  │  • Line Parser                     │  │  │
│  │  │  • Plugin Executor                 │  │  │
│  │  │  • State Updater                   │  │  │
│  │  └────────────────────────────────────┘  │  │
│  └──────────────────────────────────────────┘  │
│                                                 │
│  ┌─────────────────────────────────────────┐  │
│  │  State Management                        │  │
│  │  • Combat State Tracker                  │  │
│  │  • Entity Tracker                        │  │
│  │  • Time Keeper (NTP)                     │  │
│  └─────────────────────────────────────────┘  │
└─────────────────────────────────────────────────┘
           │
┌──────────┼──────────────────────────────────────┐
│          │    Plugin Layer                      │
│  ┌───────▼──────────────────────────────────┐  │
│  │  Built-in Plugins    External Plugins     │  │
│  │  • Timing Plugin  │  • DLL Plugin 1      │  │
│  │  • Custom Plugins │  • DLL Plugin 2      │  │
│  │                   │  • ...               │  │
│  └──────────────────────────────────────────┘  │
└─────────────────────────────────────────────────┘
```

## Component Architecture

### Core Components

#### 1. Plugin Manager (`plugin_manager`)

**Responsibilities:**
- Manages plugin lifecycle (registration, execution, cleanup)
- Coordinates parsing pipeline
- Maintains shared state
- Provides API for line processing

**Key Design:**
- Priority-based plugin execution
- Shared state via `parse_data_holder`
- Exception isolation between plugins
- Hot-reload support (DLL mode)

```cpp
// Plugin execution flow
void plugin_manager::process_line(const std::string& raw_line) {
    // 1. Parse raw line
    CombatLine line;
    parse_combat_line(raw_line, line);
    
    // 2. Execute plugins in priority order
    for (auto& plugin : sorted_plugins_) {
        plugin->ingest(shared_state_, line);
    }
    
    // 3. Update shared state
    update_combat_state(line);
    update_entity_state(line);
}
```

#### 2. Combat State Tracker (`combat_state`)

**Responsibilities:**
- Detect combat start/end
- Track combat duration
- Monitor area transitions
- Provide timing information

**State Machine:**
```
    ┌──────────┐
    │ Not in   │
    │ Combat   │
    └────┬─────┘
         │
         │ EnterCombat event
         ▼
    ┌──────────┐
    │ In       │◄─────┐
    │ Combat   │      │
    └────┬─────┘      │
         │            │
         │ No events  │ Combat events
         │ for 5s     │
         ▼            │
    ┌──────────┐      │
    │ Exiting  ├──────┘
    │ Combat   │
    └────┬─────┘
         │
         │ Timeout
         ▼
    ┌──────────┐
    │ Not in   │
    │ Combat   │
    └──────────┘
```

**Key Features:**
- Automatic combat detection
- Configurable timeout
- Area-based combat tracking
- Boss encounter detection

#### 3. Entity Tracker (`entity_tracker`)

**Responsibilities:**
- Maintain registry of all entities
- Track entity stats (HP, target, etc.)
- Provide entity lookup
- Handle entity lifecycle

**Data Structure:**
```cpp
class entity_tracker {
    std::unordered_map<uint64_t, Entity> entities_;
    Entity* owner_entity_;  // Player entity
    
    // Entity creation/retrieval
    Entity* get_or_create(uint64_t id, const std::string& name, bool is_player);
    
    // Querying
    std::vector<Entity*> all_entities();
    std::vector<Entity*> by_type(EntityType type);
};
```

**Entity Lifecycle:**
```
Created → Active → Dead → Cleanup
   │                        ↑
   └────────────────────────┘
        (Reset)
```

#### 4. Parser (`swtor_parser`)

**Responsibilities:**
- Parse raw log lines into structured data
- Extract timestamps, entities, events, values
- Validate parsing

**Parsing Strategy:**
```
Input: "[23:45:12.345] [@Player] [Ability {123}] [Event {456}] *350*"

Step 1: Extract timestamp
  → TimeStamp{h:23, m:45, s:12, ms:345}

Step 2: Parse source entity
  → Entity{name:"Player", is_player:true}

Step 3: Parse event/ability
  → Event{id:123, name:"Ability", type:ABILITY}

Step 4: Parse target and value
  → Target, Value{amount:350, is_crit:true}

Output: CombatLine struct
```

#### 5. Time Keeper (`NTPTimeKeeper`)

**Responsibilities:**
- Synchronize with NTP servers
- Provide accurate timestamps
- Handle timezone conversion
- Calculate time offsets

**Synchronization Flow:**
```
1. Query NTP servers (pool.ntp.org)
   ↓
2. Calculate network latency
   ↓
3. Compute time offset
   ↓
4. Apply to local timestamps
   ↓
5. Convert to UTC/Local
```

## Data Flow

### Line Processing Pipeline

```
Raw Log Line (String)
    │
    ▼
┌───────────────────┐
│ Line Parser       │
│ - Tokenize        │
│ - Extract fields  │
│ - Validate        │
└────────┬──────────┘
         │
         ▼
   CombatLine (Struct)
         │
         ▼
┌───────────────────┐
│ Time Cruncher     │
│ - Refine timestamp│
│ - Apply NTP offset│
│ - Handle late data│
└────────┬──────────┘
         │
         ▼
   Enhanced CombatLine
         │
         ├──────────────────────────────┐
         │                              │
         ▼                              ▼
┌────────────────────┐      ┌──────────────────────┐
│ State Updaters     │      │ Plugin Execution     │
│ - Combat state     │      │ - Priority order     │
│ - Entity tracker   │      │ - Parallel-safe      │
│ - Area tracker     │      │ - Exception isolated │
└────────────────────┘      └──────────────────────┘
         │
         ▼
   Updated State + Plugin Results
```

### State Propagation

```
parse_data_holder (Shared State)
    │
    ├─→ combat_state*        (Read: all, Write: manager)
    ├─→ entity_tracker*      (Read: all, Write: manager)
    ├─→ last_line            (Read: all, Write: manager)
    ├─→ ntp_keeper           (Read: all, Write: manager)
    └─→ time_cruncher        (Read: all, Write: manager)
         │
         ▼
    Plugins (Read-only access to shared state)
```

## Plugin System Design

### Plugin Interface Hierarchy

```
┌──────────────────────┐
│   parse_plugin       │  (Base interface)
│   - name()           │
│   - ingest()         │
│   - reset()          │
│   - set_id()         │
└──────────┬───────────┘
           │
           ├───────────────────────────┐
           │                           │
┌──────────▼───────────┐    ┌──────────▼──────────┐
│ Built-in Plugins     │    │ ExternalPluginBase  │
│ (Compile-time)       │    │ (Runtime/DLL)       │
│                      │    │ + GetInfo()         │
└──────────────────────┘    └─────────────────────┘
```

### Plugin Lifecycle

```
1. Creation
   plugin = make_shared<MyPlugin>()
   
2. Configuration
   plugin->set_priority(10)
   
3. Registration
   manager.register_plugin(plugin)
   │
   ├─→ Assign unique ID
   ├─→ Sort by priority
   └─→ Call set_id()
   
4. Execution
   For each log line:
   │
   ├─→ Parse line
   ├─→ For each plugin (sorted):
   │   └─→ plugin->ingest(data, line)
   └─→ Update state
   
5. Reset
   On combat end or manual reset:
   └─→ plugin->reset()
   
6. Destruction
   manager destroyed → plugins destroyed
```

### Plugin Priority System

```
Priority Range    Purpose              Examples
──────────────    ───────────          ────────
-1000 to -1       Pre-processing       Time adjustment
                                       Line validation
                                       
0                 Default              Most plugins
                                       
1 to 999          Application logic    DPS tracking
                                       Statistics
                                       UI updates
                                       
1000+             Post-processing      Logging
                                       File output
                                       Reports
```

## Time Synchronization

### NTP Architecture

```
┌─────────────────────────────────────────┐
│         NTPTimeKeeper                   │
│                                         │
│  ┌───────────────────────────────────┐ │
│  │  NTP Client                        │ │
│  │  • Query servers (pool.ntp.org)   │ │
│  │  • Measure round-trip time        │ │
│  │  • Calculate offset               │ │
│  └───────────────────────────────────┘ │
│                                         │
│  ┌───────────────────────────────────┐ │
│  │  Time Calculator                   │ │
│  │  • System time → UTC              │ │
│  │  • UTC → Local                    │ │
│  │  • Apply game offset              │ │
│  └───────────────────────────────────┘ │
└─────────────────────────────────────────┘
         │
         ▼
   TimeCruncher (Uses offset for refinement)
```

### Timestamp Refinement

```
Log Timestamp:  [23:45:12.345]
    │
    ▼
1. Parse components
   h=23, m=45, s=12, ms=345
    │
    ▼
2. Calculate combat_ms
   (23*3600 + 45*60 + 12)*1000 + 345
    │
    ▼
3. Determine base date
   • First line → Current date
   • Day rollover → Next day
    │
    ▼
4. Apply NTP offset
   refined_epoch_ms = base + combat_ms + ntp_offset
    │
    ▼
5. Late arrival adjustment
   If (line_time < previous_time):
       Adjust to maintain order
```

## State Management

### Combat State

```cpp
class combat_state {
    // State
    bool in_combat_;
    int64_t combat_start_;
    int64_t last_activity_;
    std::string current_area_;
    
    // Thresholds
    static constexpr int64_t COMBAT_TIMEOUT = 5000;  // 5 seconds
    
    // Transitions
    void enter_combat(const CombatLine& line);
    void exit_combat(const CombatLine& line);
    void update_activity(const CombatLine& line);
};
```

### Entity State

```cpp
struct Entity {
    // Identity
    uint64_t id;
    std::string name;
    bool is_player;
    
    // State
    HitPoints hp;
    Entity target;
    bool is_dead;
    
    // Relationships
    Entity* target_owner;  // For target's master
    std::vector<Entity*> companions;
};
```

## Design Decisions

### Why Two Architectures?

**Monolithic (Direct Integration):**
- **Pro**: Simple deployment, maximum performance
- **Pro**: Easy debugging, no DLL complexity
- **Con**: Requires recompilation for plugin changes
- **Use case**: Fixed-feature applications

**Modular (DLL):**
- **Pro**: Hot-reload plugins
- **Pro**: Third-party plugin support
- **Con**: DLL overhead, deployment complexity
- **Use case**: Extensible applications, plugin marketplace

### Why C++20?

- **std::filesystem**: Cross-platform file operations
- **std::span**: Safe array access without overhead
- **std::string_view**: Efficient string handling
- **Concepts**: Better template constraints (future)

### Why Priority-Based Execution?

Allows:
- Pre-processing before main plugins
- Post-processing after main plugins
- Deterministic execution order
- Plugin coordination

### Why Shared State?

**Alternative 1**: Pass state through function parameters
- Con: Unwieldy parameter lists
- Con: Hard to extend

**Alternative 2**: Global state
- Con: Testing difficulties
- Con: Multiple parser instances problematic

**Chosen**: Shared state holder
- Pro: Clean plugin interface
- Pro: Easy to extend
- Pro: Supports multiple parsers

## Performance Considerations

### Parse Performance

**Typical throughput**: 20,000-50,000 lines/second (Release build)

**Bottlenecks:**
1. String parsing (mitigated by efficient parsing)
2. Plugin execution (mitigated by early returns)
3. Memory allocation (mitigated by object pooling)

### Memory Usage

**Typical usage**: 100-500 MB for raid logs

**Memory profile:**
- Entity storage: ~1KB per entity
- Combat line cache: ~200 bytes per line
- Plugin state: Varies by plugin

**Optimization strategies:**
- Clear old data after processing
- Use move semantics
- Reserve vectors upfront

### Scalability

**Horizontal scaling**: Not directly supported (single-threaded)

**Vertical scaling**: 
- CPU: Scales well with single-core performance
- Memory: Scales with log size
- I/O: Bottleneck for very large files

**Future considerations:**
- Multi-threaded parsing (line batching)
- Asynchronous I/O
- Incremental processing

## Extension Points

### Adding New Event Types

1. Define event type ID in constants
2. Update parser to recognize format
3. Add to EventActionType enum
4. Document in SWTOR log reference

### Adding New Plugin Hooks

1. Define hook point in plugin interface
2. Update plugin_manager to call hook
3. Update ExternalPluginBase if needed
4. Version API (PLUGIN_API_VERSION)

### Custom State Tracking

```cpp
// Extend parse_data_holder
struct extended_parse_data : parse_data_holder {
    my_custom_tracker* custom_tracker;
};

// Use in plugins
void ingest(parse_data_holder& data, CombatLine& line) {
    auto* extended = static_cast<extended_parse_data*>(&data);
    extended->custom_tracker->update(line);
}
```

## Future Enhancements

### Planned Features

1. **Multi-threading**: Parallel plugin execution
2. **Streaming**: Process logs in real-time
3. **Compression**: Compressed log format
4. **Network sync**: Multi-client coordination
5. **Web API**: REST API for remote access

### Compatibility Considerations

- API versioning for plugins
- Backward-compatible data formats
- Migration tools for old logs

---

For implementation details:
- **API_REFERENCE.md** - API documentation
- **PLUGIN_DEVELOPMENT.md** - Plugin guide
- **BUILD_INSTRUCTIONS.md** - Build setup
- **README.md** - Project overview
