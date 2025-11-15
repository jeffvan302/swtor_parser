#pragma once
#include "swtor_parser_enhanced.h"
#include <string_view>
#include <cstdlib>

namespace swtor {
namespace detail {

// Helper to parse a NamedId from "Name {ID}" format
inline bool parse_named_id(std::string_view text, NamedId& out) {
    // Format: "Name {ID}"
    auto brace_open = text.find('{');
    if (brace_open == std::string_view::npos) return false;
    
    auto brace_close = text.find('}', brace_open);
    if (brace_close == std::string_view::npos) return false;
    
    // Extract name (trim spaces)
    auto name_part = text.substr(0, brace_open);
    while (!name_part.empty() && name_part.back() == ' ') name_part.remove_suffix(1);
    out.name = std::string(name_part);
    
    // Extract ID
    auto id_str = text.substr(brace_open + 1, brace_close - brace_open - 1);
    out.id = std::strtoull(id_str.data(), nullptr, 10);
    
    return true;
}

// Parse AreaEntered event
// Format: [AreaEntered {ID}: Area {ID} [Difficulty {ID}]] (value) <version>
// Example: [AreaEntered {836045448953664}: Nar Shaddaa {137438987989}] (he3001) <v7.0.0b>
// Example with difficulty: [AreaEntered {836045448953664}: Dxun - The CI-004 Facility {833571547775792} 8 Player Master {836045448953655}] (he3001) <v7.0.0b>
inline bool parse_area_entered(std::string_view event_text, 
                                std::string_view value_text,
                                AreaEnteredData& out) {
    // event_text should be: "AreaEntered {ID}: Area {ID} [optional difficulty]"
    
    // Find the colon separator
    auto colon_pos = event_text.find(':');
    if (colon_pos == std::string_view::npos) return false;
    
    // Skip past colon and spaces
    auto remainder = event_text.substr(colon_pos + 1);
    while (!remainder.empty() && remainder.front() == ' ') remainder.remove_prefix(1);
    
    // Check if there's a second NamedId (difficulty)
    // Find the last '}' to determine where the main area ends
    auto last_brace = remainder.rfind('}');
    if (last_brace == std::string_view::npos) return false;
    
    // Count braces to determine if we have difficulty
    size_t brace_count = 0;
    size_t first_close = 0;
    for (size_t i = 0; i < remainder.size(); ++i) {
        if (remainder[i] == '}') {
            if (brace_count == 0) first_close = i;
            brace_count++;
        }
    }
    
    if (brace_count == 1) {
        // No difficulty, just area
        if (!parse_named_id(remainder, out.area)) return false;
        out.has_difficulty = false;
    } else if (brace_count == 2) {
        // Has difficulty
        // Area is from start to first_close + 1
        auto area_part = remainder.substr(0, first_close + 1);
        if (!parse_named_id(area_part, out.area)) return false;
        
        // Difficulty is the rest (after trimming spaces)
        auto diff_part = remainder.substr(first_close + 1);
        while (!diff_part.empty() && diff_part.front() == ' ') diff_part.remove_prefix(1);
        if (!parse_named_id(diff_part, out.difficulty)) return false;
        out.has_difficulty = true;
    } else {
        return false;
    }
    
    // Parse value field (raw code like "he3001")
    if (!value_text.empty()) {
        out.raw_value = std::string(value_text);
    }
    
    // Version is parsed from threat field (angle brackets)
    // This will be handled by the main parser since it comes after the value
    
    return true;
}

// Parse version tag from threat field position
// Example: <v7.0.0b>
inline bool parse_version_tag(std::string_view text, std::string& out) {
    if (text.empty() || text.front() != '<' || text.back() != '>') return false;
    out = std::string(text.substr(1, text.size() - 2));
    return true;
}

// Parse DisciplineChanged event
// Format: [DisciplineChanged {ID}: CombatClass {ID}/Discipline {ID}]
// Example: [DisciplineChanged {836045448953665}: Operative {16140905232405801950}/Lethality {2031339142381593}]
inline bool parse_discipline_changed(std::string_view event_text, 
                                      DisciplineChangedData& out) {
    // event_text should be: "DisciplineChanged {ID}: CombatClass {ID}/Discipline {ID}"
    
    // Find the colon separator
    auto colon_pos = event_text.find(':');
    if (colon_pos == std::string_view::npos) return false;
    
    // Skip past colon and spaces
    auto remainder = event_text.substr(colon_pos + 1);
    while (!remainder.empty() && remainder.front() == ' ') remainder.remove_prefix(1);
    
    // Find the slash separator between class and discipline
    auto slash_pos = remainder.find('/');
    if (slash_pos == std::string_view::npos) return false;
    
    // Parse combat class (before slash)
    auto class_part = remainder.substr(0, slash_pos);
    if (!parse_named_id(class_part, out.combat_class)) return false;
    
    // Parse discipline (after slash)
    auto disc_part = remainder.substr(slash_pos + 1);
    if (!parse_named_id(disc_part, out.discipline)) return false;
    
    // Map IDs to enums
    out.combat_class_enum = static_cast<CombatClass>(out.combat_class.id);
    out.discipline_enum = static_cast<Discipline>(out.discipline.id);
    
    return true;
}

// Detect event kind from event type name
inline EventKind detect_event_kind(std::string_view event_name) {
    if (event_name == "ApplyEffect") return EventKind::ApplyEffect;
    if (event_name == "RemoveEffect") return EventKind::RemoveEffect;
    if (event_name == "Event") return EventKind::Event;
    if (event_name == "Spend") return EventKind::Spend;
    if (event_name == "Restore") return EventKind::Restore;
    if (event_name == "ModifyCharges") return EventKind::ModifyCharges;
    if (event_name == "AreaEntered") return EventKind::AreaEntered;
    if (event_name == "DisciplineChanged") return EventKind::DisciplineChanged;
    return EventKind::Unknown;
}

} // namespace detail

// Helper function to check if a CombatLine is an AreaEntered event
inline bool is_area_entered(const CombatLine& line) {
    return line.evt.kind == EventKind::AreaEntered;
}

// Helper function to check if a CombatLine is a DisciplineChanged event
inline bool is_discipline_changed(const CombatLine& line) {
    return line.evt.kind == EventKind::DisciplineChanged;
}

// Get area name from AreaEntered event (returns empty string if not an AreaEntered event)
inline std::string_view get_area_name(const CombatLine& line) {
    if (line.evt.kind == EventKind::AreaEntered) {
        return line.area_entered.area.name;
    }
    return "";
}

// Get difficulty name from AreaEntered event (returns empty string if not present)
inline std::string_view get_difficulty_name(const CombatLine& line) {
    if (line.evt.kind == EventKind::AreaEntered && line.area_entered.has_difficulty) {
        return line.area_entered.difficulty.name;
    }
    return "";
}

// Combat class name mapping
inline const char* combat_class_name(CombatClass cls) {
    switch (cls) {
        case CombatClass::Trooper: return "Trooper";
        case CombatClass::Smuggler: return "Smuggler";
        case CombatClass::JediKnight: return "Jedi Knight";
        case CombatClass::JediConsular: return "Jedi Consular";
        case CombatClass::BountyHunter: return "Bounty Hunter";
        case CombatClass::ImperialAgent: return "Imperial Agent";
        case CombatClass::SithWarrior: return "Sith Warrior";
        case CombatClass::SithInquisitor: return "Sith Inquisitor";
        default: return "Unknown";
    }
}

// Discipline name mapping
inline const char* discipline_name(Discipline disc) {
    switch (disc) {
        // Trooper/Bounty Hunter
        case Discipline::Gunnery: return "Gunnery";
        case Discipline::CombatMedic: return "Combat Medic";
        case Discipline::AssaultSpecialist: return "Assault Specialist";
        case Discipline::Arsenal: return "Arsenal";
        case Discipline::Bodyguard: return "Bodyguard";
        case Discipline::InnovativeOrdnance: return "Innovative Ordnance";
        case Discipline::Tactics: return "Tactics";
        case Discipline::ShieldSpecialist: return "Shield Specialist";
        case Discipline::Plasmatech: return "Plasmatech";
        case Discipline::AdvancedPrototype: return "Advanced Prototype";
        case Discipline::ShieldTech: return "Shield Tech";
        case Discipline::Pyrotech: return "Pyrotech";
        
        // Smuggler/Imperial Agent
        case Discipline::Sharpshooter: return "Sharpshooter";
        case Discipline::Saboteur: return "Saboteur";
        case Discipline::Dirty: return "Dirty Fighting";
        case Discipline::Marksmanship: return "Marksmanship";
        case Discipline::Engineering: return "Engineering";
        case Discipline::Virulence: return "Virulence";
        case Discipline::Scrapper: return "Scrapper";
        case Discipline::Sawbones: return "Sawbones";
        case Discipline::Ruffian: return "Ruffian";
        case Discipline::Concealment: return "Concealment";
        case Discipline::Lethality: return "Lethality";
        case Discipline::Medicine: return "Medicine";
        
        // Jedi Knight/Sith Warrior
        case Discipline::Watchman: return "Watchman";
        case Discipline::Combat: return "Combat";
        case Discipline::Concentration: return "Concentration";
        case Discipline::Annihilation: return "Annihilation";
        case Discipline::Carnage: return "Carnage";
        case Discipline::Fury: return "Fury";
        case Discipline::Vigilance: return "Vigilance";
        case Discipline::Defense: return "Defense";
        case Discipline::Focus: return "Focus";
        case Discipline::Vengeance: return "Vengeance";
        case Discipline::Immortal: return "Immortal";
        case Discipline::Rage: return "Rage";
        
        // Jedi Consular/Sith Inquisitor
        case Discipline::Telekinetics: return "Telekinetics";
        case Discipline::Balance: return "Balance";
        case Discipline::Seer: return "Seer";
        case Discipline::Lightning: return "Lightning";
        case Discipline::Madness: return "Madness";
        case Discipline::Corruption: return "Corruption";
        case Discipline::Infiltration: return "Infiltration";
        case Discipline::Serenity: return "Serenity";
        case Discipline::KineticCombat: return "Kinetic Combat";
        case Discipline::Deception: return "Deception";
        case Discipline::Hatred: return "Hatred";
        case Discipline::Darkness: return "Darkness";
        
        default: return "Unknown";
    }
}

} // namespace swtor
