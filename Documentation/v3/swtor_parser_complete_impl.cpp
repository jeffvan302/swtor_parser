#pragma once
#include "swtor_parser_enhanced.h"
#include <cstring>
#include <cstdlib>
#include <algorithm>

namespace swtor {
namespace detail {

// Helper to find matching closing bracket
inline size_t find_closing_bracket(std::string_view text, size_t start) {
    if (start >= text.size() || text[start] != '[') return std::string_view::npos;
    
    int depth = 0;
    for (size_t i = start; i < text.size(); ++i) {
        if (text[i] == '[') depth++;
        else if (text[i] == ']') {
            depth--;
            if (depth == 0) return i;
        }
    }
    return std::string_view::npos;
}

// Extract content between brackets at position
inline std::string_view extract_bracket_content(std::string_view text, size_t& pos) {
    if (pos >= text.size() || text[pos] != '[') {
        return {};
    }
    
    size_t close = find_closing_bracket(text, pos);
    if (close == std::string_view::npos) {
        return {};
    }
    
    std::string_view content = text.substr(pos + 1, close - pos - 1);
    pos = close + 1;
    
    // Skip whitespace after bracket
    while (pos < text.size() && text[pos] == ' ') pos++;
    
    return content;
}

// Extract content between parentheses
inline std::string_view extract_parens_content(std::string_view text, size_t& pos) {
    if (pos >= text.size() || text[pos] != '(') {
        return {};
    }
    
    size_t close = text.find(')', pos);
    if (close == std::string_view::npos) {
        return {};
    }
    
    std::string_view content = text.substr(pos + 1, close - pos - 1);
    pos = close + 1;
    
    // Skip whitespace
    while (pos < text.size() && text[pos] == ' ') pos++;
    
    return content;
}

// Extract content between angle brackets
inline std::string_view extract_angle_content(std::string_view text, size_t& pos) {
    if (pos >= text.size() || text[pos] != '<') {
        return {};
    }
    
    size_t close = text.find('>', pos);
    if (close == std::string_view::npos) {
        return {};
    }
    
    std::string_view content = text.substr(pos + 1, close - pos - 1);
    pos = close + 1;
    
    return content;
}

// Parse event field - the key function that handles special events
inline ParseStatus parse_event_field(
    std::string_view event_text,
    std::string_view value_text,
    std::string_view angle_text,
    CombatLine& out
) {
    if (event_text.empty()) {
        // Empty event field is valid (some events have no event)
        return ParseStatus::Ok;
    }
    
    // First, determine what kind of event this is by examining the text
    // Extract event type name (everything before first '{' or ':')
    size_t first_brace = event_text.find('{');
    size_t first_colon = event_text.find(':');
    
    size_t name_end = std::min(
        first_brace == std::string_view::npos ? event_text.size() : first_brace,
        first_colon == std::string_view::npos ? event_text.size() : first_colon
    );
    
    std::string_view event_type_name = event_text.substr(0, name_end);
    
    // Trim trailing spaces
    while (!event_type_name.empty() && event_type_name.back() == ' ') {
        event_type_name.remove_suffix(1);
    }
    
    // Detect event kind
    EventKind kind = detect_event_kind(event_type_name);
    out.evt.kind = kind;
    
    // Extract event kind ID (first {ID})
    if (first_brace != std::string_view::npos) {
        size_t brace_close = event_text.find('}', first_brace);
        if (brace_close != std::string_view::npos) {
            auto id_str = event_text.substr(first_brace + 1, brace_close - first_brace - 1);
            out.evt.kind_id = std::strtoull(id_str.data(), nullptr, 10);
        }
    }
    
    // Handle special event types that don't follow "EventType: Effect" pattern
    if (kind == EventKind::AreaEntered) {
        // AreaEntered format: [AreaEntered {ID}: Area {ID} [Difficulty {ID}]]
        if (!parse_area_entered(event_text, value_text, out.area_entered)) {
            return ParseStatus::Malformed;
        }
        
        // Check if angle_text contains version tag
        if (!angle_text.empty() && angle_text.front() == 'v') {
            out.area_entered.version = std::string(angle_text);
        }
        
        // Set effect for compatibility
        out.evt.effect.name = out.area_entered.area.name;
        out.evt.effect.id = out.area_entered.area.id;
        
        return ParseStatus::Ok;
        
    } else if (kind == EventKind::DisciplineChanged) {
        // DisciplineChanged format: [DisciplineChanged {ID}: Class {ID}/Discipline {ID}]
        if (!parse_discipline_changed(event_text, out.discipline_changed)) {
            return ParseStatus::Malformed;
        }
        
        // Set effect for compatibility
        out.evt.effect.name = out.discipline_changed.discipline.name;
        out.evt.effect.id = out.discipline_changed.discipline.id;
        
        return ParseStatus::Ok;
    }
    
    // For normal events with "EventType: Effect" format
    if (first_colon != std::string_view::npos) {
        auto effect_part = event_text.substr(first_colon + 1);
        
        // Trim leading spaces
        while (!effect_part.empty() && effect_part.front() == ' ') {
            effect_part.remove_prefix(1);
        }
        
        // Parse effect name and ID
        if (!parse_named_id(effect_part, out.evt.effect)) {
            return ParseStatus::Malformed;
        }
    }
    
    return ParseStatus::Ok;
}

} // namespace detail

// Modified parse_combat_line that integrates special event handling
// This is the actual implementation you need
ParseStatus parse_combat_line(std::string_view line, CombatLine& out) {
    out = CombatLine{};
    
    if (line.empty() || line.front() != '[') {
        return ParseStatus::Malformed;
    }
    
    size_t pos = 0;
    
    // 1. Parse timestamp [HH:MM:SS.mmm]
    std::string_view timestamp_text = detail::extract_bracket_content(line, pos);
    if (timestamp_text.empty()) {
        return ParseStatus::Malformed;
    }
    
    // Parse timestamp into combat_ms
    // ... your existing timestamp parsing code ...
    // out.t.combat_ms = ...
    
    // 2. Parse source [...]
    std::string_view source_text = detail::extract_bracket_content(line, pos);
    // ... your existing source parsing code ...
    // Note: source_text can be empty for some events
    
    // 3. Parse target [...]
    std::string_view target_text = detail::extract_bracket_content(line, pos);
    // ... your existing target parsing code ...
    // Note: target_text can be empty or "=" for self-target
    
    // 4. Parse ability [...]
    std::string_view ability_text = detail::extract_bracket_content(line, pos);
    // ... your existing ability parsing code ...
    // Note: ability_text can be empty
    
    // 5. Parse event [EventType {ID}: Effect {ID}] or special formats
    std::string_view event_text = detail::extract_bracket_content(line, pos);
    
    // 6. Parse optional value field (...)
    std::string_view value_text;
    if (pos < line.size() && line[pos] == '(') {
        value_text = detail::extract_parens_content(line, pos);
    }
    
    // 7. Parse optional threat/version field <...>
    std::string_view angle_text;
    if (pos < line.size() && line[pos] == '<') {
        angle_text = detail::extract_angle_content(line, pos);
    }
    
    // 8. Parse the event field - THIS IS THE KEY CHANGE
    auto status = detail::parse_event_field(event_text, value_text, angle_text, out);
    if (status != ParseStatus::Ok) {
        return status;
    }
    
    // 9. For special events, we're done
    if (out.evt.kind == EventKind::AreaEntered || 
        out.evt.kind == EventKind::DisciplineChanged) {
        return ParseStatus::Ok;
    }
    
    // 10. For normal events, parse value and threat fields
    // ... your existing value parsing code ...
    // ... your existing threat parsing code ...
    
    return ParseStatus::Ok;
}

} // namespace swtor
