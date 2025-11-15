#pragma once
#include "swtor_parser.h"
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

            }
            else if (kind == EventKind::DisciplineChanged) {
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

    bool EventPred::matches(const CombatLine& L) const {
        if (has_kind && L.evt.kind != kind) return false;
        if (has_effect_name && L.evt.effect.name != effect_name) return false;
        if (has_effect_id && L.evt.effect.id != effect_id) return false;
        return has_kind || has_effect_name || has_effect_id;
    }


    // -------- tiny utilities --------
    static inline std::string_view strip_one(std::string_view sv, char left, char right) {
        if (!sv.empty() && sv.front() == left) sv.remove_prefix(1);
        if (!sv.empty() && sv.back() == right) sv.remove_suffix(1);
        return sv;
    }

    static inline std::string_view next_bracket(std::string_view line, size_t& cursor) {
        size_t l = line.find('[', cursor);
        if (l == std::string_view::npos) return {};
        size_t r = line.find(']', l + 1);
        if (r == std::string_view::npos) return {};
        cursor = r + 1;
        return line.substr(l, r - l + 1);
    }


    // ===== Small, fast parsing utilities =====
    static inline bool fast_to_u64(std::string_view s, uint64_t& out) {
        const char* b = s.data(); const char* e = b + s.size();
        auto res = std::from_chars(b, e, out);
        return res.ec == std::errc() && res.ptr == e;
    }
    static inline bool fast_to_i64(std::string_view s, int64_t& out) {
        const char* b = s.data(); const char* e = b + s.size();
        auto res = std::from_chars(b, e, out);
        return res.ec == std::errc() && res.ptr == e;
    }
    static inline bool fast_to_u32(std::string_view s, uint32_t& out) {
        const char* b = s.data(); const char* e = b + s.size();
        auto res = std::from_chars(b, e, out);
        return res.ec == std::errc() && res.ptr == e;
    }
    static inline bool fast_to_f(std::string_view s, float& out) {
        // Fast enough and locale-safe for our use; still avoids stringstream allocations
        char* end = nullptr;
        out = std::strtof(std::string(s).c_str(), &end);
        return end && *end == '\0';
    }
    static inline void trim_ws(std::string_view& sv) {
        while (!sv.empty() && (sv.front() == ' ')) sv.remove_prefix(1);
        while (!sv.empty() && (sv.back() == ' ')) sv.remove_suffix(1);
    }
    static inline std::pair<std::string_view, std::string_view>
        split_once(std::string_view sv, char sep) {
        size_t p = sv.find(sep);
        if (p == std::string_view::npos) return { sv, std::string_view{} };
        return { sv.substr(0, p), sv.substr(p + 1) };
    }
    static inline std::string_view strip_one(std::string_view sv, char left, char right) {
        if (!sv.empty() && sv.front() == left) sv.remove_prefix(1);
        if (!sv.empty() && sv.back() == right) sv.remove_suffix(1);
        return sv;
    }
    static inline std::string_view next_bracket(std::string_view line, size_t& cursor) {
        size_t l = line.find('[', cursor);
        if (l == std::string_view::npos) return {};
        size_t r = line.find(']', l + 1);
        if (r == std::string_view::npos) return {};
        cursor = r + 1;
        return line.substr(l, r - l + 1);
    }
    static inline bool parse_signed(std::string_view s, int64_t& out) {
        const char* b = s.data(); const char* e = b + s.size();
        auto res = std::from_chars(b, e, out);
        return res.ec == std::errc() && res.ptr == e;
    }
    static inline bool parse_double_sv(std::string_view s, double& out) {
        char* end = nullptr;
        out = std::strtod(std::string(s).c_str(), &end);
        return end && *end == '\0';
    }
    static inline bool tok_eq(std::string_view t, std::string_view lit) { return t == lit; }

    // ===== Sub-parsers =====

    // [HH:MM:SS.mmm]
    static inline bool parse_timestamp_hhmmssmmm(std::string_view br, uint32_t& ms_out) {
        auto core = strip_one(br, '[', ']');
        auto [h, rest1] = split_once(core, ':');
        auto [m, rest2] = split_once(rest1, ':');
        auto [s, ms] = split_once(rest2, '.');
        uint32_t uh, um, us, ums;
        if (!fast_to_u32(h, uh) || !fast_to_u32(m, um) || !fast_to_u32(s, us) || !fast_to_u32(ms, ums))
            return false;
        ms_out = uint32_t(((uh * 60 + um) * 60 + us) * 1000 + ums);
        return true;
    }

    // (x,y,z,facing)
    static inline bool parse_position(std::string_view paren, Position& out) {
        auto core = strip_one(paren, '(', ')');
        std::array<std::string_view, 4> parts{};
        for (int i = 0; i < 4; i++) {
            auto [a, b] = split_once(core, ','); parts[i] = a; core = b;
        }
        return fast_to_f(parts[0], out.x) && fast_to_f(parts[1], out.y)
            && fast_to_f(parts[2], out.z) && fast_to_f(parts[3], out.facing);
    }

    // (current/max)
    static inline bool parse_health(std::string_view paren, Health& out) {
        auto core = strip_one(paren, '(', ')');
        auto [a, b] = split_once(core, '/');
        return fast_to_i64(a, out.current) && fast_to_i64(b, out.max);
    }

    // "Name {id}" or just "Name"
    static inline bool parse_named_id(std::string_view segment, NamedId& out) {
        size_t lb = segment.rfind('{');
        size_t rb = segment.rfind('}');
        if (lb == std::string_view::npos || rb == std::string_view::npos || rb < lb) {
            out.name = segment; out.id = 0; return true;
        }
        uint64_t id = 0; if (!fast_to_u64(segment.substr(lb + 1, rb - lb - 1), id)) return false;
        out.id = id;
        std::string_view name = segment.substr(0, lb);
        if (!name.empty() && name.back() == ' ') name.remove_suffix(1);
        out.name = name;
        return true;
    }

    // "@Name#123456" (players) — also returns non-player tokens unchanged
    static inline void parse_player_token(std::string_view sv,
        std::string_view& name_no_at,
        uint64_t& num_id,
        bool& looks_player) {
        looks_player = (!sv.empty() && sv.front() == '@');
        if (looks_player) sv.remove_prefix(1);
        size_t hash = sv.rfind('#');
        if (hash != std::string_view::npos) {
            uint64_t pid = 0;
            if (fast_to_u64(sv.substr(hash + 1), pid)) {
                num_id = pid; name_no_at = sv.substr(0, hash); return;
            }
        }
        num_id = 0; name_no_at = sv;
    }

    // [Display|Position|Health] — players, companions, NPC/objects
    static inline bool parse_entity(std::string_view br, EntityRef& out) {
        auto core = strip_one(br, '[', ']');
        out.display.assign(core.data(), core.size());

        if (core.empty()) { out.empty = true; return true; }
        if (core == "=") { out.empty = false; out.name = "="; return true; }
        out.empty = false;

        std::array<std::string_view, 3> parts{};
        for (int i = 0; i < 3; i++) { auto [a, b] = split_once(core, '|'); parts[i] = a; core = b; }
        std::string_view disp = parts[0];

        // Companion?  "OwnerToken/CompanionName {staticId[:instId]}"
        size_t slash = disp.find('/');
        if (slash != std::string_view::npos) {
            std::string_view owner_token = disp.substr(0, slash);
            std::string_view owner_name{}; uint64_t owner_id = 0; bool owner_is_player = false;
            parse_player_token(owner_token, owner_name, owner_id, owner_is_player);

            std::string_view right = disp.substr(slash + 1);
            size_t lb = right.rfind('{');
            std::string_view comp_name = right;
            uint64_t staticId = 0, instId = 0;
            if (lb != std::string_view::npos) {
                comp_name = right.substr(0, lb);
                if (!comp_name.empty() && comp_name.back() == ' ') comp_name.remove_suffix(1);
                size_t rb = right.rfind('}');
                if (rb != std::string_view::npos && rb > lb) {
                    if (!fast_to_u64(right.substr(lb + 1, rb - lb - 1), staticId)) return false;
                    size_t colon = right.find(':', rb + 1);
                    if (colon != std::string_view::npos) {
                        if (!fast_to_u64(right.substr(colon + 1), instId)) instId = 0;
                    }
                }
            }

            out.is_companion = true; out.is_player = false;
            out.owner = { std::string(owner_name), owner_id, true };
            out.name.assign(comp_name.data(), comp_name.size());
            out.companion_name = out.name;
            out.id.kind = EntityKind::NpcOrObject; out.id.hi = staticId; out.id.lo = instId;

            if (!parse_position(parts[1], out.pos)) return false;
            if (!parse_health(parts[2], out.hp))    return false;
            return true;
        }

        // Player? "@Name#123..."
        {
            std::string_view pname{}; uint64_t pid = 0; bool looks = false;
            parse_player_token(disp, pname, pid, looks);
            if (looks) {
                out.is_player = true; out.is_companion = false; out.owner = {};
                out.name.assign(pname.data(), pname.size()); out.companion_name.clear();
                out.id.kind = EntityKind::Player; out.id.hi = pid; out.id.lo = 0;
                if (!parse_position(parts[1], out.pos)) return false;
                if (!parse_health(parts[2], out.hp))    return false;
                return true;
            }
        }

        // NPC/object: "Name {staticId[:instId]}"
        {
            out.is_player = false; out.is_companion = false; out.owner = {}; out.companion_name = {};
            size_t lb = disp.rfind('{'); size_t rb = disp.rfind('}');
            uint64_t staticId = 0, instId = 0;
            if (lb != std::string_view::npos && rb != std::string_view::npos && rb > lb) {
                if (!fast_to_u64(disp.substr(lb + 1, rb - lb - 1), staticId)) return false;
                size_t colon = disp.find(':', rb + 1);
                if (colon != std::string_view::npos) {
                    if (!fast_to_u64(disp.substr(colon + 1), instId)) instId = 0;
                }
                std::string_view nm = disp.substr(0, lb);
                if (!nm.empty() && nm.back() == ' ') nm.remove_suffix(1);
                out.name.assign(nm.data(), nm.size());
            }
            else {
                out.name.assign(disp.data(), disp.size());
            }
            out.id.kind = EntityKind::NpcOrObject; out.id.hi = staticId; out.id.lo = instId;

            if (!parse_position(parts[1], out.pos)) return false;
            if (!parse_health(parts[2], out.hp))    return false;
            return true;
        }
    }

    // [Name {id}]
    static inline bool parse_ability(std::string_view br, NamedId& out) {
        auto core = strip_one(br, '[', ']'); return parse_named_id(core, out);
    }

    // ----- Trailing helpers -----
    static inline bool peel_terminal_threat(std::string_view& tail, double& threat, bool& present) {
        present = false;
        trim_ws(tail);
        if (tail.empty() || tail.back() != '>') return true;
        size_t lb = tail.rfind('<');
        if (lb == std::string_view::npos) return true;
        std::string_view inner = tail.substr(lb + 1, tail.size() - lb - 2);
        trim_ws(inner);
        double t = 0.0;
        if (parse_double_sv(inner, t)) { threat = t; present = true; }
        tail = tail.substr(0, lb);
        trim_ws(tail);
        return true;
    }
    static inline std::string_view peel_paren_group(std::string_view& sv) {
        trim_ws(sv);
        if (sv.empty() || sv.front() != '(') return {};
        int depth = 0; size_t i = 0;
        for (; i < sv.size(); ++i) {
            if (sv[i] == '(') ++depth;
            else if (sv[i] == ')') { if (--depth == 0) { ++i; break; } }
        }
        if (depth != 0) return {};
        std::string_view group = sv.substr(1, i - 2);
        sv.remove_prefix(i); trim_ws(sv);
        return group;
    }
    static inline bool has_flag(MitigationFlags f, MitigationFlags b) {
        return (static_cast<uint16_t>(f) & static_cast<uint16_t>(b)) != 0;
    }
    static inline void parse_mitigation_tail(std::string_view rest, ValueField& vf) {
        std::string_view cur = rest;
        while (!cur.empty() && cur.front() == '-') {
            cur.remove_prefix(1);
            size_t stop = cur.find_first_of(" {");
            std::string_view token = (stop == std::string_view::npos) ? cur : cur.substr(0, stop);
            if (tok_eq(token, "shield")) vf.mitig |= MitigationFlags::Shield;
            else if (tok_eq(token, "deflect")) vf.mitig |= MitigationFlags::Deflect;
            else if (tok_eq(token, "glance"))  vf.mitig |= MitigationFlags::Glance;
            else if (tok_eq(token, "dodge"))   vf.mitig |= MitigationFlags::Dodge;
            else if (tok_eq(token, "parry"))   vf.mitig |= MitigationFlags::Parry;
            else if (tok_eq(token, "resist"))  vf.mitig |= MitigationFlags::Resist;
            else if (tok_eq(token, "miss"))    vf.mitig |= MitigationFlags::Miss;
            else if (tok_eq(token, "immune"))  vf.mitig |= MitigationFlags::Immune;

            if (stop == std::string_view::npos) { cur = {}; break; }
            cur.remove_prefix(stop); trim_ws(cur);

            // Optional "{shield_effect_id}"
            if (!cur.empty() && cur.front() == '{') {
                size_t rb = cur.find('}');
                if (rb != std::string_view::npos) {
                    std::string_view idsv = cur.substr(1, rb - 1);
                    uint64_t sid = 0; if (fast_to_u64(idsv, sid)) {
                        if (has_flag(vf.mitig, MitigationFlags::Shield)) {
                            vf.shield.present = true; vf.shield.shield_effect_id = sid;
                        }
                    }
                    cur.remove_prefix(rb + 1); trim_ws(cur);
                }
            }
            // Optional "(123 absorbed {id})"
            if (!cur.empty() && cur.front() == '(') {
                auto grp = peel_paren_group(cur);
                auto [x, rest2] = split_once(grp, ' ');
                if (tok_eq(rest2.substr(0, 8), "absorbed")) {
                    int64_t absorbed = 0; if (parse_signed(x, absorbed)) {
                        vf.shield.present = true; vf.shield.absorbed = absorbed;
                    }
                    size_t lb = rest2.find('{'); size_t rb = rest2.find('}', lb + 1);
                    if (lb != std::string_view::npos && rb != std::string_view::npos) {
                        uint64_t aid = 0; if (fast_to_u64(rest2.substr(lb + 1, rb - lb - 1), aid)) {
                            vf.shield.present = true; vf.shield.absorbed_id = aid;
                        }
                    }
                }
                trim_ws(cur);
            }
            while (!cur.empty() && cur.front() == ' ') cur.remove_prefix(1);
            if (!cur.empty() && cur.front() != '-') break;
        }
    }
    static inline bool parse_value_group(std::string_view grp, ValueField& out) {
        std::string_view cur = grp; trim_ws(cur);

        // amount
        size_t stop = cur.find_first_of(" *~");
        std::string_view amt = (stop == std::string_view::npos) ? cur : cur.substr(0, stop);
        int64_t amount = 0; if (!parse_signed(amt, amount)) return false;
        out.amount = amount;
        if (stop == std::string_view::npos) cur = {};
        else cur.remove_prefix(stop);

        // crit marker
        if (!cur.empty() && cur.front() == '*') { out.crit = true; cur.remove_prefix(1); }
        trim_ws(cur);

        // "~ sec" secondary value
        if (!cur.empty() && cur.front() == '~') {
            cur.remove_prefix(1); trim_ws(cur);
            size_t sstop = cur.find_first_of(" )");
            std::string_view sec = (sstop == std::string_view::npos) ? cur : cur.substr(0, sstop);
            int64_t s = 0; if (!parse_signed(sec, s)) return false;
            out.has_secondary = true; out.secondary = s;
            if (sstop == std::string_view::npos) cur = {};
            else cur.remove_prefix(sstop);
        }
        trim_ws(cur);

        // optional school [+ {id}]
        if (!cur.empty() && cur.front() != '-' && cur.front() != '(') {
            size_t wstop = cur.find_first_of(" {");
            if (wstop == std::string_view::npos) wstop = cur.size();
            std::string_view w = cur.substr(0, wstop);
            cur.remove_prefix(wstop); trim_ws(cur);

            uint64_t sid = 0;
            if (!cur.empty() && cur.front() == '{') {
                size_t rb = cur.find('}');
                if (rb != std::string_view::npos) {
                    std::string_view idsv = cur.substr(1, rb - 1);
                    if (fast_to_u64(idsv, sid)) {
                        out.school.present = true; out.school.name = w; out.school.id = sid;
                    }
                    cur.remove_prefix(rb + 1); trim_ws(cur);
                }
                else {
                    out.school.present = true; out.school.name = w;
                }
            }
            else if (!w.empty()) {
                out.school.present = true; out.school.name = w;
            }
        }

        // mitigation chain (zero or more)
        if (!cur.empty() && cur.front() == '-') parse_mitigation_tail(cur, out);
        return true;
    }
    static inline bool parse_charges_group(std::string_view grp, int32_t& charges) {
        std::string_view core = grp; trim_ws(core);
        auto [numtok, rest] = split_once(core, ' ');
        int64_t n = 0; if (!parse_signed(numtok, n)) return false;
        trim_ws(rest);
        if (rest != "charges") return false;
        charges = static_cast<int32_t>(n);
        return true;
    }

    // Fast-path: "(amt[*]? [~ sec]? school {id}?) [<threat>]" with no '-' or extra '('
    static inline bool parse_trailing_fast_common(std::string_view tail, Trailing& out) {
        std::string_view work = tail;
        double threat = 0.0; bool hasThreat = false;
        if (!peel_terminal_threat(work, threat, hasThreat)) return false;

        if (work.find('-') != std::string_view::npos) return false;
        size_t first_open = work.find('(');
        if (first_open == std::string_view::npos) {
            out.kind = ValueKind::None; out.has_threat = hasThreat; out.threat = threat; return true;
        }
        size_t second_open = work.find('(', first_open + 1);
        if (second_open != std::string_view::npos) return false;

        size_t close = work.find(')', first_open + 1);
        if (close == std::string_view::npos) return false;
        std::string_view grp = work.substr(first_open + 1, close - (first_open + 1));

        ValueField vf{};
        const char* p = grp.data();
        const char* end = p + grp.size();

        // amount
        bool neg = (p < end && *p == '-'); if (neg) ++p;
        int64_t amount = 0; const char* startAmt = p;
        while (p < end && *p >= '0' && *p <= '9') { amount = amount * 10 + (*p - '0'); ++p; }
        if (p == startAmt) return false;
        if (neg) amount = -amount;
        vf.amount = amount;
        while (p < end && *p == ' ') ++p;

        // crit
        if (p < end && *p == '*') { vf.crit = true; ++p; while (p < end && *p == ' ') ++p; }

        // "~ sec"
        if (p < end && *p == '~') {
            ++p; while (p < end && *p == ' ') ++p;
            bool sneg = (p < end && *p == '-'); if (sneg) ++p;
            int64_t sec = 0; const char* s0 = p;
            while (p < end && *p >= '0' && *p <= '9') { sec = sec * 10 + (*p - '0'); ++p; }
            if (p == s0) return false;
            if (sneg) sec = -sec;
            vf.has_secondary = true; vf.secondary = sec;
            while (p < end && *p == ' ') ++p;
        }

        // school [ + optional {id} ]
        const char* schoolStart = p;
        while (p < end && ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z'))) ++p;
        if (p > schoolStart) {
            vf.school.present = true;
            vf.school.name = std::string_view(schoolStart, static_cast<size_t>(p - schoolStart));
            while (p < end && *p == ' ') ++p;
            if (p < end && *p == '{') {
                ++p; uint64_t sid = 0; const char* id0 = p;
                while (p < end && *p >= '0' && *p <= '9') { sid = sid * 10 + (*p - '0'); ++p; }
                if (p == id0 || p == end || *p != '}') return false;
                ++p;
                vf.school.id = sid;
            }
            while (p < end && *p == ' ') ++p;
        }

        if (p != end) return false;

        out.kind = ValueKind::Numeric; out.val = vf;
        out.has_threat = hasThreat; out.threat = threat;
        out.unparsed = work.substr(close + 1); // leftover spacing, if any (usually empty)
        return true;
    }




    // -------- timestamp [HH:MM:SS.mmm] --------
    static inline bool parse_timestamp_hhmmssmmm(std::string_view br, uint32_t& ms_out) {
        auto core = strip_one(br, '[', ']');
        auto [h, rest1] = split_once(core, ':');
        auto [m, rest2] = split_once(rest1, ':');
        auto [s, ms] = split_once(rest2, '.');

        uint32_t uh, um, us, ums;
        if (!fast_to_u32(h, uh)) return false;
        if (!fast_to_u32(m, um)) return false;
        if (!fast_to_u32(s, us)) return false;
        if (!fast_to_u32(ms, ums)) return false;

        ms_out = uint32_t(((uh * 60 + um) * 60 + us) * 1000 + ums);
        return true;
    }

    // -------- entity [Display|Position|Health] --------
    // Supports players (@Name#id), companions (Owner/Companion {staticId:instId}),
    // and NPCs/objects (Name {staticId:instId}).
    static inline bool parse_entity(std::string_view br, EntityRef& out) {
        auto core = strip_one(br, '[', ']');
        out.display.assign(core.data(), core.size());

        if (core.empty()) { out.empty = true; return true; }
        if (core == "=") { out.empty = false; out.name = "="; return true; }
        out.empty = false;

        std::array<std::string_view, 3> parts{};
        for (int i = 0; i < 3; i++) { auto [a, b] = split_once(core, '|'); parts[i] = a; core = b; }
        std::string_view disp = parts[0];

        // Companion: "OwnerToken/CompanionName {staticId[:instId]}"
        if (size_t slash = disp.find('/'); slash != std::string_view::npos) {
            std::string_view owner_token = disp.substr(0, slash);
            std::string_view owner_name{}; uint64_t owner_id = 0; bool owner_is_player = false;
            parse_player_token(owner_token, owner_name, owner_id, owner_is_player);

            std::string_view right = disp.substr(slash + 1);
            size_t lb = right.rfind('{'); std::string_view comp_name = right;
            uint64_t staticId = 0, instId = 0;
            if (lb != std::string_view::npos) {
                comp_name = right.substr(0, lb);
                if (!comp_name.empty() && comp_name.back() == ' ') comp_name.remove_suffix(1);
                size_t rb = right.rfind('}');
                if (rb != std::string_view::npos && rb > lb) {
                    if (!fast_to_u64(right.substr(lb + 1, rb - lb - 1), staticId)) return false;
                    size_t colon = right.find(':', rb + 1);
                    if (colon != std::string_view::npos) {
                        if (!fast_to_u64(right.substr(colon + 1), instId)) instId = 0;
                    }
                }
            }

            out.is_companion = true; out.is_player = false;
            out.owner = { std::string(owner_name), owner_id, true };
            out.name.assign(comp_name.data(), comp_name.size());
            out.companion_name = out.name;
            out.id.kind = EntityKind::NpcOrObject; out.id.hi = staticId; out.id.lo = instId;

            if (!parse_position(parts[1], out.pos)) return false;
            if (!parse_health(parts[2], out.hp))    return false;
            return true;
        }

        // Player: "@Name#1234…"
        {
            std::string_view pname{}; uint64_t pid = 0; bool looks = false;
            parse_player_token(disp, pname, pid, looks);
            if (looks) {
                out.is_player = true; out.is_companion = false; out.owner = {};
                out.name.assign(pname.data(), pname.size()); out.companion_name.clear();
                out.id.kind = EntityKind::Player; out.id.hi = pid; out.id.lo = 0;
                if (!parse_position(parts[1], out.pos)) return false;
                if (!parse_health(parts[2], out.hp))    return false;
                return true;
            }
        }

        // NPC/object: "Name {staticId[:instId]}"
        {
            out.is_player = false; out.is_companion = false; out.owner = {}; out.companion_name = {};
            size_t lb = disp.rfind('{'); size_t rb = disp.rfind('}');
            uint64_t staticId = 0, instId = 0;
            if (lb != std::string_view::npos && rb != std::string_view::npos && rb > lb) {
                if (!fast_to_u64(disp.substr(lb + 1, rb - lb - 1), staticId)) return false;
                size_t colon = disp.find(':', rb + 1);
                if (colon != std::string_view::npos) {
                    if (!fast_to_u64(disp.substr(colon + 1), instId)) instId = 0;
                }
                std::string_view nm = disp.substr(0, lb);
                if (!nm.empty() && nm.back() == ' ') nm.remove_suffix(1);
                out.name.assign(nm.data(), nm.size());
            }
            else {
                out.name.assign(disp.data(), disp.size());
            }
            out.id.kind = EntityKind::NpcOrObject; out.id.hi = staticId; out.id.lo = instId;

            if (!parse_position(parts[1], out.pos)) return false;
            if (!parse_health(parts[2], out.hp))    return false;
            return true;
        }
    }

    // -------- ability [Name {id}] --------
    static inline bool parse_ability(std::string_view br, NamedId& out) {
        auto core = strip_one(br, '[', ']');
        return parse_named_id(core, out);
    }

    // -------- trailing (value/charges/mitigation/threat) --------
    // Uses a fast-path for the common "(amount [*] [~ sec] school {id}) [<threat>]"
    // and falls back to the tolerant parser for everything else.
    static inline bool parse_trailing(std::string_view tail, Trailing& out) {
        out = {};
        if (parse_trailing_fast_common(tail, out)) return true;

        double thr = 0.0; bool hasThr = false;
        if (!peel_terminal_threat(tail, thr, hasThr)) return false;
        out.has_threat = hasThr; out.threat = thr;

        if (tail.empty()) { out.kind = ValueKind::None; return true; }

        auto grp1 = peel_paren_group(tail);
        if (grp1.empty()) { out.kind = ValueKind::None; out.unparsed = tail; return true; }

        int32_t charges = 0;
        if (parse_charges_group(grp1, charges)) {
            out.kind = ValueKind::Charges; out.charges = charges; out.has_charges = true;
            out.unparsed = tail; return true;
        }

        ValueField vf{};
        if (!parse_value_group(grp1, vf)) {
            out.kind = ValueKind::Unknown;
            out.unparsed = grp1;
            return true; // tolerant
        }
        out.kind = ValueKind::Numeric; out.val = vf;
        out.unparsed = tail;
        return true;
    }


    // Modified parse_combat_line that integrates special event handling
    // This is the actual implementation you need
// ===== Public parser =====
    ParseStatus parse_combat_line(std::string_view line, CombatLine& out) {
        out = CombatLine{};

        size_t cur = 0;

        // [HH:MM:SS.mmm]
        auto tsbr = next_bracket(line, cur);            // returns "[..]"
        if (tsbr.empty()) return ParseStatus::Malformed;
        if (!parse_timestamp_hhmmssmmm(tsbr, out.t.combat_ms))   // fast, alloc-free
            return ParseStatus::Malformed; // :contentReference[oaicite:0]{index=0}

        // [source]
        auto srcbr = next_bracket(line, cur);
        if (srcbr.empty()) return ParseStatus::Malformed;
        if (!parse_entity(srcbr, out.source))
            return ParseStatus::Malformed; // :contentReference[oaicite:1]{index=1}

        // [target]
        auto tgtbr = next_bracket(line, cur);
        if (tgtbr.empty()) return ParseStatus::Malformed;
        if (!parse_entity(tgtbr, out.target))
            return ParseStatus::Malformed; // :contentReference[oaicite:2]{index=2}
        if (!out.target.empty && out.target.name == "=") {
            // SWTOR self-target shorthand ("=") — mirror source quickly
            out.target = out.source; // :contentReference[oaicite:3]{index=3}
        }

        // [ability]
        auto abbr = next_bracket(line, cur);
        if (abbr.empty()) return ParseStatus::Malformed;
        if (!parse_ability(abbr, out.ability))
            return ParseStatus::Malformed; // :contentReference[oaicite:4]{index=4}

        // [event]  (special formats handled)
        auto evbr = next_bracket(line, cur);
        if (evbr.empty()) return ParseStatus::Malformed;

        // Prepare the optional trailing groups so special events can read them
        // ( ) value group and < > (threat or version) — alloc-free peel
        // We don't advance 'cur' when probing; we use local 'probe' to read groups.
        size_t probe = cur;
        while (probe < line.size() && line[probe] == ' ') ++probe;

        std::string_view value_text{};
        if (probe < line.size() && line[probe] == '(') {
            value_text = detail::extract_parens_content(line, probe); // "(...)" -> inner
        }                                                             // :contentReference[oaicite:5]{index=5}

        std::string_view angle_text{};
        if (probe < line.size() && line[probe] == '<') {
            angle_text = detail::extract_angle_content(line, probe);  // "<...>" -> inner
        }                                                             // :contentReference[oaicite:6]{index=6}

        // Feed the event core (without brackets) + optional groups to the new handler
        std::string_view event_core = strip_one(evbr, '[', ']');
        if (auto st = detail::parse_event_field(event_core, value_text, angle_text, out);
            st != ParseStatus::Ok) {
            return st; // handles AreaEntered & DisciplineChanged, sets evt.kind/effect, etc.
        } // :contentReference[oaicite:7]{index=7} :contentReference[oaicite:8]{index=8}

        // If it's one of the special events, we're done. (Version tag lives in angle_text.)
        if (out.evt.kind == EventKind::AreaEntered || out.evt.kind == EventKind::DisciplineChanged) {
            return ParseStatus::Ok; // :contentReference[oaicite:9]{index=9}
        }

        // Parse trailing value/charges/mitigation/threat (fast path + tolerant fallback)
        std::string_view tailSV;
        if (cur < line.size()) {
            tailSV = line.substr(cur);
            while (!tailSV.empty() && tailSV.front() == ' ') tailSV.remove_prefix(1);
        }
        if (!parse_trailing(tailSV, out.tail))  // parses (..), charges, mitigation, and <threat>
            return ParseStatus::Malformed;      // :contentReference[oaicite:10]{index=10}

        return ParseStatus::Ok;
    }


} // namespace swtor
