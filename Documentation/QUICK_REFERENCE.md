# SWTOR Parser Enhancement - Quick Reference Card

## Detection (All Three Methods Work)

```cpp
// ✅ Recommended: Using EventPred (most flexible)
if (line == swtor::Evt::AreaEntered) { /* ... */ }
if (line == swtor::Evt::DisciplineChanged) { /* ... */ }

// ✅ Using EventTag (for simple cases)
if (line == swtor::Events::AreaEntered) { /* ... */ }
if (line == swtor::Events::DisciplineChanged) { /* ... */ }

// ✅ Using helper functions (clearest intent)
if (swtor::is_area_entered(line)) { /* ... */ }
if (swtor::is_discipline_changed(line)) { /* ... */ }
```

## Accessing Data

### AreaEntered
```cpp
if (line == swtor::Evt::AreaEntered) {
    // Required fields
    std::string area_name = line.area_entered.area.name;
    uint64_t area_id = line.area_entered.area.id;
    std::string version = line.area_entered.version;
    
    // Optional fields
    if (line.area_entered.has_difficulty) {
        std::string diff_name = line.area_entered.difficulty.name;
        uint64_t diff_id = line.area_entered.difficulty.id;
    }
    
    // Quick access helpers
    auto area = swtor::get_area_name(line);        // Returns area name or ""
    auto diff = swtor::get_difficulty_name(line);  // Returns difficulty or ""
}
```

### DisciplineChanged
```cpp
if (line == swtor::Evt::DisciplineChanged) {
    // String data
    std::string class_name = line.discipline_changed.combat_class.name;
    std::string disc_name = line.discipline_changed.discipline.name;
    
    // Numeric IDs
    uint64_t class_id = line.discipline_changed.combat_class.id;
    uint64_t disc_id = line.discipline_changed.discipline.id;
    
    // Enum values (for type-safe comparisons)
    CombatClass cls = line.discipline_changed.combat_class_enum;
    Discipline disc = line.discipline_changed.discipline_enum;
    
    // Get human-readable names from enums
    const char* class_str = swtor::combat_class_name(cls);
    const char* disc_str = swtor::discipline_name(disc);
}
```

## Combined Predicates

```cpp
// Match multiple event types
auto special = swtor::Evt::AreaEntered | swtor::Evt::DisciplineChanged;
if (line == special) { /* matches either */ }

// Match damage OR heal
auto combat = swtor::Evt::Damage | swtor::Evt::Heal;
if (line == combat) { /* matches either */ }
```

## Switch Statements

```cpp
switch (line.evt.kind) {
    case swtor::EventKind::AreaEntered:
        std::cout << "Entered: " << line.area_entered.area.name << std::endl;
        break;
        
    case swtor::EventKind::DisciplineChanged:
        std::cout << "Changed to: " << line.discipline_changed.discipline.name << std::endl;
        break;
        
    case swtor::EventKind::ApplyEffect:
        // Check effect type
        if (line == swtor::Evt::Damage) { /* damage */ }
        else if (line == swtor::Evt::Heal) { /* heal */ }
        break;
        
    default:
        break;
}
```

## Combat Classes

```cpp
// Enum values
CombatClass::Trooper
CombatClass::Smuggler
CombatClass::JediKnight
CombatClass::JediConsular
CombatClass::BountyHunter
CombatClass::ImperialAgent
CombatClass::SithWarrior
CombatClass::SithInquisitor

// Get name
const char* name = swtor::combat_class_name(CombatClass::Operative);
// Returns: "Imperial Agent" (note: Operative is a combat style, returns base class)
```

## Disciplines (Common Ones)

```cpp
// Tanks
Discipline::ShieldSpecialist  // Vanguard/Powertech
Discipline::Defense           // Guardian/Juggernaut
Discipline::KineticCombat     // Shadow/Assassin

// Healers
Discipline::CombatMedic      // Commando/Mercenary
Discipline::Sawbones         // Scoundrel/Operative
Discipline::Seer             // Sage/Sorcerer

// DPS (examples)
Discipline::Lethality        // Operative
Discipline::Gunnery          // Commando
Discipline::Fury             // Marauder
Discipline::Lightning        // Sorcerer

// Get name
const char* name = swtor::discipline_name(Discipline::Lethality);
// Returns: "Lethality"
```

## Filtering Logs

```cpp
void process_log(const std::vector<swtor::CombatLine>& lines) {
    for (const auto& line : lines) {
        // Count area changes
        if (line == swtor::Evt::AreaEntered) {
            area_count++;
        }
        
        // Track discipline changes
        if (line == swtor::Evt::DisciplineChanged) {
            current_class = line.discipline_changed.combat_class_enum;
            current_disc = line.discipline_changed.discipline_enum;
        }
        
        // Process damage with current loadout context
        if (line == swtor::Evt::Damage) {
            // You know what class/disc the player was using
            record_damage(line, current_class, current_disc);
        }
    }
}
```

## TimeCruncher Integration

```cpp
// OLD way (string search)
bool TimeCruncher::isAreaEntered(const CombatLine& line) const {
    return std::string_view(line.evt.effect.name).find("AreaEntered") 
           != std::string_view::npos;
}

// NEW way (enum check - much faster!)
bool TimeCruncher::isAreaEntered(const CombatLine& line) const {
    return line.evt.kind == EventKind::AreaEntered;
}
```

## Example Log Lines

```
Simple area:
[23:54:32.862] [@Player|(...)|(...)] [] [] 
[AreaEntered {836045448953664}: Nar Shaddaa {137438987989}] (he3001) <v7.0.0b>

Area with difficulty:
[21:37:53.211] [@Player|(...)|(...)] [] [] 
[AreaEntered {836045448953664}: Dxun - The CI-004 Facility {833571547775792} 8 Player Master {836045448953655}] 
(he3001) <v7.0.0b>

Discipline change:
[21:31:26.309] [@Player|(...)|(...)] [] [] 
[DisciplineChanged {836045448953665}: Operative {16140905232405801950}/Lethality {2031339142381593}]
```

## Most Common Pattern

```cpp
swtor::CombatLine line;
if (swtor::parse_combat_line(raw_line, line) == swtor::ParseStatus::Ok) {
    
    if (line == swtor::Evt::AreaEntered) {
        // Zone changed
        log_area_change(line.area_entered.area.name, 
                       line.area_entered.difficulty.name);
    }
    
    else if (line == swtor::Evt::DisciplineChanged) {
        // Loadout changed
        update_player_spec(line.discipline_changed.combat_class_enum,
                          line.discipline_changed.discipline_enum);
    }
    
    else if (line == swtor::Evt::Damage) {
        // Combat event
        record_damage(line.tail.val.amount);
    }
}
```

## Type Checking

```cpp
// At compile time
static_assert(std::is_enum_v<swtor::EventKind>);
static_assert(std::is_enum_v<swtor::CombatClass>);
static_assert(std::is_enum_v<swtor::Discipline>);

// Runtime checks
if (line.evt.kind != EventKind::Unknown) {
    // Valid event
}

if (line.discipline_changed.combat_class_enum != CombatClass::Unknown) {
    // Valid class
}
```

## Performance Notes

1. **Enum comparisons are fast** - O(1) integer comparison
2. **No string allocations** - All string_views reference parsed data
3. **Zero-copy parsing** - Original line data is referenced, not copied
4. **Type-safe** - Compile-time checking of event types

## Common Pitfalls

❌ **Don't do this:**
```cpp
if (line.evt.effect.name == "AreaEntered") // Won't work for new events
```

✅ **Do this instead:**
```cpp
if (line == swtor::Evt::AreaEntered)  // Correct!
```

❌ **Don't do this:**
```cpp
if (line.area_entered.area.name.empty()) // May crash if not AreaEntered
```

✅ **Do this instead:**
```cpp
if (line == swtor::Evt::AreaEntered && !line.area_entered.area.name.empty())
```

## See Also

- `README.md` - Complete feature documentation
- `INTEGRATION_GUIDE.txt` - Step-by-step integration
- `example_enhanced_usage.cpp` - Working code examples
- `swtor_parser_enhanced.h` - Full API reference
