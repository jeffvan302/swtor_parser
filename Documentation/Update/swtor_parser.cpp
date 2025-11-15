#include "swtor_parser.h"

#include <array>
#include <charconv>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string_view>
#include <vector>
#include <chrono>


std::string swtor::to_text(const swtor::CombatLine& L, const swtor::PrintOptions& opt) {
    std::ostringstream o;
    auto ent = [&](std::string_view label, const swtor::EntityRef& e) {
        o << label << (opt.multiline ? ": " : " : ");
        o << (e.is_player ? "PLAYER " : (e.is_companion ? "COMPANION " : "NPC "));
        o << '"' << e.name << '"';
        if (e.is_companion && e.owner.has_owner) {
            o << " owner=" << e.owner.name_no_at;
            if (e.owner.player_numeric_id) o << "#" << e.owner.player_numeric_id;
        }
        o << " id[" << (int)e.id.kind << ":" << e.id.hi << ":" << e.id.lo << "]";
        if (opt.include_positions) {
            o << " pos(" << e.pos.x << "," << e.pos.y << "," << e.pos.z << "," << e.pos.facing << ")";
        }
        if (opt.include_health) {
            o << " hp(" << e.hp.current << "/" << e.hp.max << ")";
        }
        if (opt.multiline) o << "\n"; else o << "  ";
        };

    if (opt.multiline) {
        o << "time: " << L.t.combat_ms << " ms";
        if (L.t.refined_epoch_ms >= 0) o << " (epoch=" << L.t.refined_epoch_ms << ")";
        o << "\n";
        ent("src", L.source);
        ent("tgt", L.target);
        o << "ability: " << L.ability.name << "{" << L.ability.id << "}\n";
        o << "event: " << swtor::kind_to_cstr(L.evt.kind) << "{" << L.evt.kind_id << "}";
        if (!L.evt.effect.name.empty() || L.evt.effect.id) {
            o << " : " << L.evt.effect.name << "{" << L.evt.effect.id << "}";
        }
        o << "\n";

        o << "tail: ";
        switch (L.tail.kind) {
        case swtor::ValueKind::None:    o << "None"; break;
        case swtor::ValueKind::Charges: o << "charges=" << L.tail.charges; break;
        case swtor::ValueKind::Unknown: o << "Unknown(" << L.tail.unparsed << ")"; break;
        case swtor::ValueKind::Numeric: {
            const auto& v = L.tail.val;
            o << "amount=" << v.amount;
            if (v.crit) o << " crit";
            if (v.has_secondary) o << " secondary=" << v.secondary;
            if (v.school.present) o << " school=" << v.school.name << "{" << v.school.id << "}";
            if (v.mitig != swtor::MitigationFlags::None) {
                o << " mitig=" << swtor::flags_to_string(v.mitig);
                if (v.shield.present) {
                    o << " shield{id=" << v.shield.shield_effect_id
                        << " absorbed=" << v.shield.absorbed
                        << " absorbed_id=" << v.shield.absorbed_id << "}";
                }
            }
        } break;
        }
        if (L.tail.has_threat) o << " threat=" << std::fixed << std::setprecision(1) << L.tail.threat;
        if (!L.tail.unparsed.empty()) o << " unparsed='" << L.tail.unparsed << "'";
        o << "\n";
    }
    else {
        o << "t=" << L.t.combat_ms << "ms  ";
        ent("SRC", L.source);
        ent("TGT", L.target);
        o << "ABIL " << L.ability.name << "{" << L.ability.id << "}  ";
        o << "EVT " << swtor::kind_to_cstr(L.evt.kind) << "{" << L.evt.kind_id << "}";
        if (!L.evt.effect.name.empty() || L.evt.effect.id) {
            o << ":" << L.evt.effect.name << "{" << L.evt.effect.id << "}";
        }
        o << "  ";
        switch (L.tail.kind) {
        case swtor::ValueKind::None:    o << "tail=None"; break;
        case swtor::ValueKind::Charges: o << "tail=Charges(" << L.tail.charges << ")"; break;
        case swtor::ValueKind::Unknown: o << "tail=Unknown"; break;
        case swtor::ValueKind::Numeric: {
            o << "tail=Value(amount=" << L.tail.val.amount;
            if (L.tail.val.crit) o << ", crit";
            if (L.tail.val.has_secondary) o << ", secondary=" << L.tail.val.secondary;
            if (L.tail.val.school.present) {
                o << ", " << L.tail.val.school.name << "{" << L.tail.val.school.id << "}";
            }
            o << ")";
            break;
        }
        }
        if (L.tail.has_threat) o << " threat=" << std::fixed << std::setprecision(1) << L.tail.threat;
    }
    return o.str();
}


namespace swtor {

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
        char* end = nullptr;
        out = std::strtof(std::string(s).c_str(), &end);
        return end && *end == '\0';
    }
    static inline void trim_ws(std::string_view& sv) {
        while (!sv.empty() && (sv.front() == ' ')) sv.remove_prefix(1);
        while (!sv.empty() && (sv.back() == ' '))  sv.remove_suffix(1);
    }
    static inline std::pair<std::string_view, std::string_view>
        split_once(std::string_view sv, char sep) {
        size_t p = sv.find(sep);
        if (p == std::string_view::npos) return { sv, std::string_view{} };
        return { sv.substr(0,p), sv.substr(p + 1) };
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
    static inline bool parse_position(std::string_view paren, Position& out) {
        auto core = strip_one(paren, '(', ')');
        std::array<std::string_view, 4> parts{};
        for (int i = 0; i < 4; i++) {
            auto [a, b] = split_once(core, ','); parts[i] = a; core = b;
        }
        return fast_to_f(parts[0], out.x) && fast_to_f(parts[1], out.y)
            && fast_to_f(parts[2], out.z) && fast_to_f(parts[3], out.facing);
    }
    static inline bool parse_health(std::string_view paren, Health& out) {
        auto core = strip_one(paren, '(', ')');
        auto [a, b] = split_once(core, '/');
        return fast_to_i64(a, out.current) && fast_to_i64(b, out.max);
    }
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
    static inline void parse_player_token(std::string_view sv, std::string_view& name_no_at, uint64_t& num_id, bool& looks_player) {
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

    // Entity (players / NPCs / companions)
    static inline bool parse_entity(std::string_view br, EntityRef& out) {
        auto core = strip_one(br, '[', ']');
        out.display.assign(core.data(), core.size());

        if (core.empty()) { out.empty = true; return true; }
        if (core == "=") { out.empty = false; out.name = "="; return true; }
        out.empty = false;

        std::array<std::string_view, 3> parts{};
        for (int i = 0; i < 3; i++) { auto [a, b] = split_once(core, '|'); parts[i] = a; core = b; }
        std::string_view disp = parts[0];

        // Companion?
        size_t slash = disp.find('/');
        if (slash != std::string_view::npos) {
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

        // Player?
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

        // NPC
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

    static inline bool parse_ability(std::string_view br, NamedId& out) {
        auto core = strip_one(br, '[', ']'); return parse_named_id(core, out);
    }
    static inline bool parse_event_effect(std::string_view br, EventEffect& out) {
        auto core = strip_one(br, '[', ']');
        auto [left, right] = split_once(core, ':');
        NamedId leftni{}; if (!parse_named_id(left, leftni)) return false;
        out.kind_id = leftni.id;
        auto lname = leftni.name;
        auto eq = [&](std::string_view s) { return lname == s; };
        if (eq("ApplyEffect"))    out.kind = EventKind::ApplyEffect;
        else if (eq("RemoveEffect"))   out.kind = EventKind::RemoveEffect;
        else if (eq("Event"))          out.kind = EventKind::Event;
        else if (eq("Spend"))          out.kind = EventKind::Spend;
        else if (eq("Restore"))        out.kind = EventKind::Restore;
        else if (eq("ModifyCharges"))  out.kind = EventKind::ModifyCharges;
        else                           out.kind = EventKind::Unknown;

        if (!right.empty() && right.front() == ' ') right.remove_prefix(1);
        if (!right.empty()) {
            if (!parse_named_id(right, out.effect)) return false;
        }
        else {
            out.effect = {};
        }
        return true;
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
            else if (tok_eq(token, "deflect"))vf.mitig |= MitigationFlags::Deflect;
            else if (tok_eq(token, "glance")) vf.mitig |= MitigationFlags::Glance;
            else if (tok_eq(token, "dodge"))  vf.mitig |= MitigationFlags::Dodge;
            else if (tok_eq(token, "parry"))  vf.mitig |= MitigationFlags::Parry;
            else if (tok_eq(token, "resist")) vf.mitig |= MitigationFlags::Resist;
            else if (tok_eq(token, "miss"))   vf.mitig |= MitigationFlags::Miss;
            else if (tok_eq(token, "immune")) vf.mitig |= MitigationFlags::Immune;

            if (stop == std::string_view::npos) { cur = {}; break; }
            cur.remove_prefix(stop); trim_ws(cur);

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

        size_t stop = cur.find_first_of(" *~");
        std::string_view amt = (stop == std::string_view::npos) ? cur : cur.substr(0, stop);
        int64_t amount = 0; if (!parse_signed(amt, amount)) return false;
        out.amount = amount;
        if (stop == std::string_view::npos) cur = {};
        else cur.remove_prefix(stop);

        if (!cur.empty() && cur.front() == '*') { out.crit = true; cur.remove_prefix(1); }
        trim_ws(cur);

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

        bool neg = (p < end && *p == '-'); if (neg) ++p;
        int64_t amount = 0; const char* startAmt = p;
        while (p < end && *p >= '0' && *p <= '9') { amount = amount * 10 + (*p - '0'); ++p; }
        if (p == startAmt) return false;
        if (neg) amount = -amount;
        vf.amount = amount;

        while (p < end && *p == ' ') ++p;
        if (p < end && *p == '*') { vf.crit = true; ++p; while (p < end && *p == ' ') ++p; }

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
        out.unparsed = {};
        return true;
    }

    // Full trailing parse (with fast path)
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

    // ===== Public parser =====
    ParseStatus parse_combat_line(std::string_view line, CombatLine& out) {
        size_t cur = 0;

        auto tsbr = next_bracket(line, cur); if (tsbr.empty()) return ParseStatus::Malformed;
        if (!parse_timestamp_hhmmssmmm(tsbr, out.t.combat_ms)) return ParseStatus::Malformed;

        auto srcbr = next_bracket(line, cur); if (srcbr.empty()) return ParseStatus::Malformed;
        if (!parse_entity(srcbr, out.source)) return ParseStatus::Malformed;

        auto tgtbr = next_bracket(line, cur); if (tgtbr.empty()) return ParseStatus::Malformed;
        if (!parse_entity(tgtbr, out.target)) return ParseStatus::Malformed;
        if (!out.target.empty && out.target.name == "=") { out.target = out.source; }

        auto abbr = next_bracket(line, cur); if (abbr.empty()) return ParseStatus::Malformed;
        if (!parse_ability(abbr, out.ability)) return ParseStatus::Malformed;

        auto evbr = next_bracket(line, cur); if (evbr.empty()) return ParseStatus::Malformed;
        if (!parse_event_effect(evbr, out.evt)) return ParseStatus::Malformed;

        std::string_view tailSV;
        if (cur < line.size()) {
            tailSV = line.substr(cur);
            while (!tailSV.empty() && tailSV.front() == ' ') tailSV.remove_prefix(1);
        }
        if (!parse_trailing(tailSV, out.tail)) return ParseStatus::Malformed;

        return ParseStatus::Ok;
    }

    // ===== Pretty printer =====
    const char* kind_to_cstr(EventKind k) {
        switch (k) {
        case EventKind::ApplyEffect: return "ApplyEffect";
        case EventKind::RemoveEffect: return "RemoveEffect";
        case EventKind::Event: return "Event";
        case EventKind::Spend: return "Spend";
        case EventKind::Restore: return "Restore";
        case EventKind::ModifyCharges: return "ModifyCharges";
        default: return "Unknown";
        }
    }
    std::string flags_to_string(MitigationFlags f) {
        if (f == MitigationFlags::None) return "None";
        std::ostringstream oss; bool first = true;
        auto add = [&](const char* n) { if (!first) oss << '|'; first = false; oss << n; };
        if ((static_cast<uint16_t>(f) & static_cast<uint16_t>(MitigationFlags::Shield)))  add("Shield");
        if ((static_cast<uint16_t>(f) & static_cast<uint16_t>(MitigationFlags::Deflect))) add("Deflect");
        if ((static_cast<uint16_t>(f) & static_cast<uint16_t>(MitigationFlags::Glance)))  add("Glance");
        if ((static_cast<uint16_t>(f) & static_cast<uint16_t>(MitigationFlags::Dodge)))   add("Dodge");
        if ((static_cast<uint16_t>(f) & static_cast<uint16_t>(MitigationFlags::Parry)))   add("Parry");
        if ((static_cast<uint16_t>(f) & static_cast<uint16_t>(MitigationFlags::Resist)))  add("Resist");
        if ((static_cast<uint16_t>(f) & static_cast<uint16_t>(MitigationFlags::Miss)))    add("Miss");
        if ((static_cast<uint16_t>(f) & static_cast<uint16_t>(MitigationFlags::Immune)))  add("Immune");
        return oss.str();
    }

    // ===== EventPred matcher =====
    bool EventPred::matches(const CombatLine& L) const {
        // Check kind if specified
        if (has_kind && L.evt.kind != kind) {
            return false;
        }

        // Check effect name if specified
        if (has_effect_name && L.evt.effect.name != effect_name) {
            return false;
        }

        // Check effect ID if specified
        if (has_effect_id && L.evt.effect.id != effect_id) {
            return false;
        }

        // All specified conditions matched
        return true;
    }

    // ===== JSON (emit & parse) =====
    namespace detail_json {
        static inline void jescape(std::string_view s, std::string& out) {
            out.push_back('"');
            for (char c : s) {
                switch (c) {
                case '\"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\b': out += "\\b"; break;
                case '\f': out += "\\f"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20) {
                        char buf[7];
                        std::snprintf(buf, sizeof(buf), "\\u%04x", (unsigned char)c);
                        out += buf;
                    }
                    else out.push_back(c);
                }
            }
            out.push_back('"');
        }
        static inline void skip_ws(std::string_view& s) { while (!s.empty() && (s.front() == ' ' || s.front() == '\n' || s.front() == '\r' || s.front() == '\t')) s.remove_prefix(1); }
        static inline bool eat(std::string_view& s, char c) { skip_ws(s); if (!s.empty() && s.front() == c) { s.remove_prefix(1); return true; } return false; }
        static inline bool take_key(std::string_view& s, std::string_view key) {
            skip_ws(s);
            if (s.size() < key.size() + 3) return false;
            if (s.front() != '\"') return false;
            size_t pos = 1;
            for (; pos < s.size(); ++pos) {
                if (s[pos] == '\"' && s[pos - 1] != '\\') break;
            }
            if (pos >= s.size()) return false;
            auto k = s.substr(1, pos - 1);
            if (k != key) return false;
            s.remove_prefix(pos + 1); skip_ws(s);
            if (!eat(s, ':')) return false;
            return true;
        }
        static inline bool take_string(std::string_view& s, std::string& out) {
            skip_ws(s);
            if (s.empty() || s.front() != '\"') return false;
            size_t pos = 1;
            for (; pos < s.size(); ++pos) {
                if (s[pos] == '\"' && s[pos - 1] != '\\') break;
            }
            if (pos >= s.size()) return false;
            auto token = s.substr(1, pos - 1);
            s.remove_prefix(pos + 1);
            out.clear(); out.reserve(token.size());
            for (size_t i = 0; i < token.size(); ++i) {
                char c = token[i];
                if (c == '\\' && i + 1 < token.size()) {
                    char e = token[++i];
                    switch (e) {
                    case '\"': out.push_back('\"'); break;
                    case '\\': out.push_back('\\'); break;
                    case 'b': out.push_back('\b'); break;
                    case 'f': out.push_back('\f'); break;
                    case 'n': out.push_back('\n'); break;
                    case 'r': out.push_back('\r'); break;
                    case 't': out.push_back('\t'); break;
                    default: return false;
                    }
                }
                else out.push_back(c);
            }
            return true;
        }
        static inline bool take_number(std::string_view& s, int64_t& out) {
            skip_ws(s);
            size_t i = 0; if (i < s.size() && (s[i] == '-' || s[i] == '+')) ++i;
            size_t j = i; while (j < s.size() && std::isdigit((unsigned char)s[j])) ++j;
            if (i == j) return false;
            auto tok = s.substr(0, j);
            s.remove_prefix(j);
            return fast_to_i64(tok, out);
        }
        static inline bool take_u64(std::string_view& s, uint64_t& out) {
            skip_ws(s);
            size_t j = 0; while (j < s.size() && std::isdigit((unsigned char)s[j])) ++j;
            if (j == 0) return false;
            auto tok = s.substr(0, j);
            s.remove_prefix(j);
            return fast_to_u64(tok, out);
        }
        static inline bool take_double(std::string_view& s, double& out) {
            skip_ws(s);
            size_t j = 0;
            if (j < s.size() && (s[j] == '+' || s[j] == '-')) ++j;
            bool dot = false;
            while (j < s.size() && (std::isdigit((unsigned char)s[j]) || (!dot && s[j] == '.'))) { dot |= (s[j] == '.'); ++j; }
            if (j == 0) return false;
            auto tok = s.substr(0, j);
            s.remove_prefix(j);
            return parse_double_sv(tok, out);
        }
    } // namespace detail_json

    std::string to_json(const CombatLine& L) {
        using namespace detail_json;
        std::string j; j.reserve(256);
        j += "{";

        j += "\"t_ms\":"; j += std::to_string(L.t.combat_ms);
        if (L.t.refined_epoch_ms >= 0) { j += ",\"t_epoch\":"; j += std::to_string(L.t.refined_epoch_ms); }

        auto j_entity = [&](const char* key, const EntityRef& e) {
            j += ",\""; j += key; j += "\":{";
            j += "\"name\":"; jescape(e.name, j);
            if (e.is_companion) { j += ",\"companion\":"; jescape(e.companion_name, j); }
            if (e.owner.has_owner) {
                j += ",\"owner\":"; jescape(e.owner.name_no_at, j);
                if (e.owner.player_numeric_id) { j += ",\"owner_id\":"; j += std::to_string(e.owner.player_numeric_id); }
            }
            j += ",\"kind\":"; j += std::to_string((int)e.id.kind);
            if (e.id.hi) { j += ",\"id_hi\":"; j += std::to_string(e.id.hi); }
            if (e.id.lo) { j += ",\"id_lo\":"; j += std::to_string(e.id.lo); }
            if (e.is_player) j += ",\"is_player\":true";
            if (e.is_companion) j += ",\"is_companion\":true";
            j += "}";
            };
        j_entity("src", L.source);
        j_entity("tgt", L.target);

        j += ",\"ability\":{";
        j += "\"name\":"; jescape(L.ability.name, j);
        if (L.ability.id) { j += ",\"id\":"; j += std::to_string(L.ability.id); }
        j += "}";

        j += ",\"event\":{";
        j += "\"kind\":"; jescape(kind_to_cstr(L.evt.kind), j);
        if (L.evt.kind_id) { j += ",\"kind_id\":"; j += std::to_string(L.evt.kind_id); }
        if (!L.evt.effect.name.empty() || L.evt.effect.id) {
            j += ",\"effect\":{";
            j += "\"name\":"; jescape(L.evt.effect.name, j);
            if (L.evt.effect.id) { j += ",\"id\":"; j += std::to_string(L.evt.effect.id); }
            j += "}";
        }
        j += "}";

        j += ",\"tail\":{";
        switch (L.tail.kind) {
        case ValueKind::None:    j += "\"kind\":\"None\""; break;
        case ValueKind::Charges: j += "\"kind\":\"Charges\",\"charges\":" + std::to_string(L.tail.charges); break;
        case ValueKind::Unknown: j += "\"kind\":\"Unknown\""; break;
        case ValueKind::Numeric: {
            j += "\"kind\":\"Numeric\",\"val\":{";
            const auto& v = L.tail.val;
            j += "\"amount\":" + std::to_string(v.amount);
            if (v.crit) j += ",\"crit\":true";
            if (v.has_secondary) j += ",\"secondary\":" + std::to_string(v.secondary);
            if (v.school.present) {
                j += ",\"school\":{";
                j += "\"name\":"; jescape(v.school.name, j);
                if (v.school.id) j += ",\"id\":" + std::to_string(v.school.id);
                j += "}";
            }
            if (v.mitig != MitigationFlags::None) {
                j += ",\"mitig\":" + std::to_string((int)v.mitig);
                if (v.shield.present) {
                    j += ",\"shield\":{";
                    if (v.shield.shield_effect_id) j += "\"id\":" + std::to_string(v.shield.shield_effect_id) + ",";
                    j += "\"absorbed\":" + std::to_string(v.shield.absorbed);
                    if (v.shield.absorbed_id) j += ",\"absorbed_id\":" + std::to_string(v.shield.absorbed_id);
                    j += "}";
                }
            }
            j += "}";
        } break;
        }
        if (L.tail.has_threat) {
            std::ostringstream os; os.setf(std::ios::fixed); os << std::setprecision(1) << L.tail.threat;
            j += ",\"threat\":" + os.str();
        }
        j += "}";

        j += "}";
        return j;
    }

    bool from_json(std::string_view json, CombatLine& out, detail_json::StringArena& arena) {
        using namespace detail_json;
        out = {};
        std::string tmp;

        skip_ws(json); if (!eat(json, '{')) return false;

        // t_ms
        if (!take_key(json, "t_ms")) return false;
        { int64_t ms = 0; if (!take_number(json, ms)) return false; out.t.combat_ms = (uint32_t)ms; }

        // optional t_epoch
        skip_ws(json);
        if (eat(json, ',')) {
            std::string_view probe = json;
            if (take_key(probe, "t_epoch")) {
                int64_t epoch = 0; if (!take_number(probe, epoch)) return false;
                out.t.refined_epoch_ms = epoch;
                json = probe;
            }
            else {
                json = std::string_view(json.data() - 1, json.size() + 1);
                eat(json, ',');
            }
        }

        auto rd_entity = [&](std::string_view key, EntityRef& e)->bool {
            if (!take_key(json, key)) return false;
            if (!eat(json, '{')) return false;

            // name
            if (!take_key(json, "name")) return false;
            if (!take_string(json, tmp)) return false;
            e.name = arena.intern(tmp);

            // optional companion
            skip_ws(json);
            if (eat(json, ',')) {
                std::string_view probe = json;
                if (take_key(probe, "companion")) {
                    if (!take_string(probe, tmp)) return false;
                    e.is_companion = true;
                    e.companion_name = arena.intern(tmp);
                    json = probe;
                }
                else {
                    json = std::string_view(json.data() - 1, json.size() + 1);
                    eat(json, ',');
                }
            }

            // optional owner(+owner_id)
            skip_ws(json);
            if (eat(json, ',')) {
                std::string_view probe = json;
                if (take_key(probe, "owner")) {
                    if (!take_string(probe, tmp)) return false;
                    e.owner.name_no_at = arena.intern(tmp);
                    e.owner.has_owner = true;
                    if (eat(probe, ',')) {
                        if (take_key(probe, "owner_id")) {
                            uint64_t pid = 0; if (!take_u64(probe, pid)) return false;
                            e.owner.player_numeric_id = pid;
                        }
                        else return false;
                    }
                    json = probe;
                }
                else {
                    json = std::string_view(json.data() - 1, json.size() + 1);
                    eat(json, ',');
                }
            }

            // kind
            if (!take_key(json, "kind")) return false;
            int64_t kind = 0; if (!take_number(json, kind)) return false;
            e.id.kind = (EntityKind)kind;

            // optional id_hi / id_lo
            skip_ws(json);
            if (eat(json, ',')) {
                std::string_view probe = json;
                if (take_key(probe, "id_hi")) {
                    uint64_t hi = 0; if (!take_u64(probe, hi)) return false; e.id.hi = hi;
                    json = probe;
                    skip_ws(json);
                    if (eat(json, ',')) {
                        std::string_view probe2 = json;
                        if (take_key(probe2, "id_lo")) {
                            uint64_t lo = 0; if (!take_u64(probe2, lo)) return false; e.id.lo = lo; json = probe2;
                        }
                        else {
                            json = std::string_view(json.data() - 1, json.size() + 1);
                            eat(json, ',');
                        }
                    }
                }
                else if (take_key(probe, "id_lo")) {
                    uint64_t lo = 0; if (!take_u64(probe, lo)) return false; e.id.lo = lo; json = probe;
                }
                else {
                    json = std::string_view(json.data() - 1, json.size() + 1);
                    eat(json, ',');
                }
            }

            // optional flags
            skip_ws(json);
            if (eat(json, ',')) {
                std::string_view probe = json;
                if (take_key(probe, "is_player")) {
                    if (!eat(probe, 't') || !eat(probe, 'r') || !eat(probe, 'u') || !eat(probe, 'e')) return false;
                    e.is_player = true;
                    json = probe;
                    skip_ws(json);
                    if (eat(json, ',')) {
                        std::string_view probe2 = json;
                        if (take_key(probe2, "is_companion")) {
                            if (!eat(probe2, 't') || !eat(probe2, 'r') || !eat(probe2, 'u') || !eat(probe2, 'e')) return false;
                            e.is_companion = true;
                            json = probe2;
                        }
                        else {
                            json = std::string_view(json.data() - 1, json.size() + 1);
                            eat(json, ',');
                        }
                    }
                }
                else if (take_key(probe, "is_companion")) {
                    if (!eat(probe, 't') || !eat(probe, 'r') || !eat(probe, 'u') || !eat(probe, 'e')) return false;
                    e.is_companion = true; json = probe;
                }
                else {
                    json = std::string_view(json.data() - 1, json.size() + 1);
                    eat(json, ',');
                }
            }

            if (!eat(json, '}')) return false;
            return true;
            };

        if (!rd_entity("src", out.source)) return false;
        if (!eat(json, ',')) return false;
        if (!rd_entity("tgt", out.target)) return false;

        // ability
        if (!eat(json, ',')) return false;
        if (!take_key(json, "ability")) return false;
        if (!eat(json, '{')) return false;
        if (!take_key(json, "name")) return false;
        if (!take_string(json, tmp)) return false;
        out.ability.name = arena.intern(tmp);
        if (eat(json, ',')) {
            if (!take_key(json, "id")) return false;
            uint64_t id = 0; if (!take_u64(json, id)) return false; out.ability.id = id;
        }
        if (!eat(json, '}')) return false;

        // event
        if (!eat(json, ',')) return false;
        if (!take_key(json, "event")) return false;
        if (!eat(json, '{')) return false;
        if (!take_key(json, "kind")) return false;
        if (!take_string(json, tmp)) return false;
        if (tmp == "ApplyEffect") out.evt.kind = EventKind::ApplyEffect;
        else if (tmp == "RemoveEffect")out.evt.kind = EventKind::RemoveEffect;
        else if (tmp == "Event")       out.evt.kind = EventKind::Event;
        else if (tmp == "Spend")       out.evt.kind = EventKind::Spend;
        else if (tmp == "Restore")     out.evt.kind = EventKind::Restore;
        else if (tmp == "ModifyCharges") out.evt.kind = EventKind::ModifyCharges;
        else out.evt.kind = EventKind::Unknown;

        if (eat(json, ',')) {
            std::string_view probe = json;
            if (take_key(probe, "kind_id")) {
                uint64_t kid = 0; if (!take_u64(probe, kid)) return false;
                out.evt.kind_id = kid; json = probe;
            }
            else {
                json = std::string_view(json.data() - 1, json.size() + 1);
                eat(json, ',');
            }
        }
        if (take_key(json, "effect")) {
            if (!eat(json, '{')) return false;
            if (!take_key(json, "name")) return false;
            if (!take_string(json, tmp)) return false;
            out.evt.effect.name = arena.intern(tmp);
            if (eat(json, ',')) {
                if (!take_key(json, "id")) return false;
                uint64_t id = 0; if (!take_u64(json, id)) return false; out.evt.effect.id = id;
            }
            if (!eat(json, '}')) return false;
        }
        if (!eat(json, '}')) return false;

        // tail
        if (!eat(json, ',')) return false;
        if (!take_key(json, "tail")) return false;
        if (!eat(json, '{')) return false;
        if (!take_key(json, "kind")) return false;
        if (!take_string(json, tmp)) return false;
        if (tmp == "None") out.tail.kind = ValueKind::None;
        else if (tmp == "Charges") out.tail.kind = ValueKind::Charges;
        else if (tmp == "Numeric") out.tail.kind = ValueKind::Numeric;
        else out.tail.kind = ValueKind::Unknown;

        if (out.tail.kind == ValueKind::Charges) {
            if (eat(json, ',')) { if (!take_key(json, "charges")) return false; int64_t c = 0; if (!take_number(json, c)) return false; out.tail.charges = (int32_t)c; }
        }
        else if (out.tail.kind == ValueKind::Numeric) {
            if (!eat(json, ',')) return false;
            if (!take_key(json, "val")) return false;
            if (!eat(json, '{')) return false;
            if (!take_key(json, "amount")) return false;
            int64_t amt = 0; if (!take_number(json, amt)) return false;
            out.tail.val.amount = amt;

            if (eat(json, ',')) {
                std::string_view p = json;
                if (take_key(p, "crit")) { if (!eat(p, 't') || !eat(p, 'r') || !eat(p, 'u') || !eat(p, 'e')) return false; out.tail.val.crit = true; json = p; if (eat(json, ',')) {} }
            }
            if (take_key(json, "secondary")) { int64_t sec = 0; if (!take_number(json, sec)) return false; out.tail.val.has_secondary = true; out.tail.val.secondary = sec; if (eat(json, ',')) {} }
            if (take_key(json, "school")) {
                if (!eat(json, '{')) return false;
                if (!take_key(json, "name")) return false;
                if (!take_string(json, tmp)) return false;
                out.tail.val.school.present = true;
                out.tail.val.school.name = arena.intern(tmp);
                if (eat(json, ',')) { if (!take_key(json, "id")) return false; uint64_t sid = 0; if (!take_u64(json, sid)) return false; out.tail.val.school.id = sid; }
                if (!eat(json, '}')) return false;
                if (eat(json, ',')) {}
            }
            if (take_key(json, "mitig")) { int64_t m = 0; if (!take_number(json, m)) return false; out.tail.val.mitig = (MitigationFlags)m; if (eat(json, ',')) {} }
            if (take_key(json, "shield")) {
                if (!eat(json, '{')) return false;
                std::string_view p = json;
                if (take_key(p, "id")) { uint64_t sid = 0; if (!take_u64(p, sid)) return false; out.tail.val.shield.present = true; out.tail.val.shield.shield_effect_id = sid; json = p; if (eat(json, ',')) {} }
                if (!take_key(json, "absorbed")) return false;
                int64_t ab = 0; if (!take_number(json, ab)) return false;
                out.tail.val.shield.present = true; out.tail.val.shield.absorbed = ab;
                if (eat(json, ',')) { if (!take_key(json, "absorbed_id")) return false; uint64_t aid = 0; if (!take_u64(json, aid)) return false; out.tail.val.shield.absorbed_id = aid; }
                if (!eat(json, '}')) return false;
            }
            if (!eat(json, '}')) return false; // end val
        }

        // optional threat
        if (eat(json, ',')) {
            std::string_view p = json;
            if (take_key(p, "threat")) {
                double t = 0.0; if (!take_double(p, t)) return false;
                out.tail.has_threat = true; out.tail.threat = t;
                json = p;
            }
            else {
                json = std::string_view(json.data() - 1, json.size() + 1);
                eat(json, ',');
            }
        }

        if (!eat(json, '}')) return false; // end tail
        skip_ws(json); if (!eat(json, '}')) return false; // end root
        skip_ws(json); if (!json.empty()) return false;

        return true;
    }

    // ===== Ownership helpers =====
    namespace {
        static inline std::string_view intern_or_empty(swtor::detail_json::StringArena& a, std::string_view s) {
            if (s.empty()) return {};
            return a.intern(s);
        }
    } // anonymous

    void deep_bind_into(CombatLine& dst, const CombatLine& src, detail_json::StringArena& arena) {
        dst = {}; // start clean

        // Time
        dst.t = src.t;

        // Source
        dst.source.is_player = src.source.is_player;
        dst.source.is_companion = src.source.is_companion;
        dst.source.empty = src.source.empty;
        dst.source.id = src.source.id;
        dst.source.pos = src.source.pos;
        dst.source.hp = src.source.hp;

        dst.source.display = intern_or_empty(arena, src.source.display);
        dst.source.name = intern_or_empty(arena, src.source.name);
        dst.source.companion_name = intern_or_empty(arena, src.source.companion_name);

        dst.source.owner.has_owner = src.source.owner.has_owner;
        dst.source.owner.player_numeric_id = src.source.owner.player_numeric_id;
        dst.source.owner.name_no_at = intern_or_empty(arena, src.source.owner.name_no_at);

        // Target
        dst.target.is_player = src.target.is_player;
        dst.target.is_companion = src.target.is_companion;
        dst.target.empty = src.target.empty;
        dst.target.id = src.target.id;
        dst.target.pos = src.target.pos;
        dst.target.hp = src.target.hp;

        dst.target.display = intern_or_empty(arena, src.target.display);
        dst.target.name = intern_or_empty(arena, src.target.name);
        dst.target.companion_name = intern_or_empty(arena, src.target.companion_name);

        dst.target.owner.has_owner = src.target.owner.has_owner;
        dst.target.owner.player_numeric_id = src.target.owner.player_numeric_id;
        dst.target.owner.name_no_at = intern_or_empty(arena, src.target.owner.name_no_at);

        // Ability
        dst.ability.id = src.ability.id;
        dst.ability.name = intern_or_empty(arena, src.ability.name);

        // Event / Effect
        dst.evt.kind = src.evt.kind;
        dst.evt.kind_id = src.evt.kind_id;
        dst.evt.effect.id = src.evt.effect.id;
        dst.evt.effect.name = intern_or_empty(arena, src.evt.effect.name);

        // Trailing
        dst.tail.kind = src.tail.kind;
        dst.tail.has_charges = src.tail.has_charges;
        dst.tail.charges = src.tail.charges;
        dst.tail.has_threat = src.tail.has_threat;
        dst.tail.threat = src.tail.threat;
        dst.tail.unparsed = intern_or_empty(arena, src.tail.unparsed);

        dst.tail.val.amount = src.tail.val.amount;
        dst.tail.val.crit = src.tail.val.crit;
        dst.tail.val.has_secondary = src.tail.val.has_secondary;
        dst.tail.val.secondary = src.tail.val.secondary;
        dst.tail.val.mitig = src.tail.val.mitig;

        dst.tail.val.shield.present = src.tail.val.shield.present;
        dst.tail.val.shield.shield_effect_id = src.tail.val.shield.shield_effect_id;
        dst.tail.val.shield.absorbed = src.tail.val.shield.absorbed;
        dst.tail.val.shield.absorbed_id = src.tail.val.shield.absorbed_id;

        dst.tail.val.school.present = src.tail.val.school.present;
        dst.tail.val.school.id = src.tail.val.school.id;
        dst.tail.val.school.name = intern_or_empty(arena, src.tail.val.school.name);
    }

    OwnedCombatLine make_owned_clone(const CombatLine& src) { return OwnedCombatLine(src); }

} // namespace swtor


#define SWTOR_PARSER_DEMO_MAIN
// ===== Optional demo main =====
#ifdef SWTOR_PARSER_DEMO_MAIN
int main(int argc, char** argv) {
    using namespace std;
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " /path/to/combat_log.txt\n";
        return 1;
    }
    const string path = argv[1];

    cout << "SWTOR Combat Log Parser Demo\n";

    vector<string> lines;
    {
        ifstream in(path, ios::in);
        if (!in) { cerr << "Error: cannot open file: " << path << "\n"; return 1; }
        string s;
        while (getline(in, s)) {
            if (!s.empty() && s.back() == '\r') s.pop_back();
            lines.push_back(move(s));
        }
    }
    if (lines.empty()) { cerr << "File has 0 lines.\n"; return 0; }
    else {
        cout << "Loaded " << lines.size() << " lines from file.\n";
    }

    const size_t total = lines.size();
    size_t parsed_ok = 0;
    swtor::OwnedCombatLine keep;
	string keep_str = "";
    bool is_first = true;
    auto t0 = std::chrono::steady_clock::now();
    for (const auto& line : lines) {
        swtor::CombatLine L{};
        auto st = swtor::parse_combat_line(std::string_view{ line }, L);
        if (st == swtor::ParseStatus::Ok) {
            if (is_first && L.source.is_player && L == swtor::Evt::Damage) {
                is_first = false;
                keep = swtor::make_owned_clone(L);
                keep_str = line;
            }

            ++parsed_ok;
        }
    }
    auto t1 = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> ms = t1 - t0;

    const double total_ms = ms.count();
    const double avg_ms_per_line = total_ms / static_cast<double>(total);
    const double lines_per_sec = (total_ms > 0.0) ? (1000.0 / avg_ms_per_line) : 0.0;

    cout << fixed << setprecision(3);
    cout << "File: " << path << "\n";
    cout << "Lines total: " << total << "\n";
    cout << "Lines parsed OK: " << parsed_ok << "\n";
    cout << "Elapsed: " << total_ms << " ms\n";
    cout << "Average: " << avg_ms_per_line << " ms/line\n";
    cout << "Throughput: " << lines_per_sec << " lines/sec\n";
	cout << "\n";
	cout << "Sample line (kept):\n" << keep_str << "\n";
    std::cout << "Sample (kept):\n" << swtor::to_text(keep.value) << "\n";
    return 0;
}
#endif
