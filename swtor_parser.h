#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <cstdlib>

namespace swtor {


    // Add the two special “by-name only” kinds as numeric IDs, observed in logs/spec:
    inline constexpr uint64_t KINDID_AreaEntered = 836045448953664ULL;
    inline constexpr uint64_t KINDID_DisciplineChanged = 836045448953665ULL;

    // If you already had the earlier constants, keep them together:
    inline constexpr uint64_t KINDID_Event = 836045448945472ULL;
    inline constexpr uint64_t KINDID_Spend = 836045448945473ULL;
    inline constexpr uint64_t KINDID_Restore = 836045448945476ULL;
    inline constexpr uint64_t KINDID_ApplyEffect = 836045448945477ULL;
    inline constexpr uint64_t KINDID_RemoveEffect = 836045448945478ULL;
    inline constexpr uint64_t KINDID_ModifyCharges = 836045448953666ULL;

    // ---------- Combat Classes and Disciplines ----------

    // Base combat classes (8 original classes mapped to Combat Styles in 7.0+)
    enum class CombatClass : uint64_t {
        Unknown = 0,

        // Republic Tech
        Trooper = 16140999253208197512,
        Smuggler = 16140997055451521365,

        // Republic Force
        JediKnight = 16141007844876951097,
        JediConsular = 16140903134212196743,

        // Empire Tech  
        BountyHunter = 16140911277033332389,
        ImperialAgent = 16140905232405801950,

        // Empire Force
        SithWarrior = 16141153526575710780,
        SithInquisitor = 16141122432429723681
    };

    enum class EventType: uint64_t {
        Unknown = 0,
        Event = 836045448945472,
        AreaEntered = 836045448953664,
        Spend = 836045448945473,
        DisciplineChanged = 836045448953665,
        ApplyEffect = 836045448945477,
        RemoveEffect = 836045448945478,
        ModifyCharges = 836045448953666,
        Restore = 836045448945476
	};

    enum class EventActionType : uint64_t {
        Unknown = 0,
        Heal = 836045448945500,
        EnterCombat = 836045448945489,
        ExitCombat = 836045448945490,
        Damage = 836045448945501,
        FailedEffect = 836045448945499,
        Revived = 836045448945494,
        ModifyThreat = 836045448945483,
        FallingDamage = 836045448945484,
        Death = 836045448945493,
        TargetSet= 836045448953668,
        TargetCleared = 836045448953669,
        AbilityActivate  = 836045448945479,
        AbilityInterrupt  = 836045448945482,
        AbilityDeactivate = 836045448945480,
        AbilityCancel = 836045448945481,
        energy = 836045448938503,
        LeaveCover = 836045448945486,
        Crouch = 836045448945487
    };

    enum class CombatStyle : uint64_t {
        Unknown = 0,
        // Trooper Styles
        Commando = 3088803483451153,
        Mercenary = 594992886408417,
        // Smuggler Styles
        Gunslinger = 3508869182982329,
        Sniper = 3225114604527897,
        // Jedi Knight Styles
        Sentinel = 3508879977426105,
        Marauder = 3219155620896953,
        // Jedi Consular Styles
        Sage = 1944553467445561,
        Sorcerer = 3300941827327161,
        // Bounty Hunter Styles
        Vanguard = 1944502563654937,
        Powertech = 3320456030634169,
        // Imperial Agent Styles
        Scoundrel = 2487504318513465,
        Operative = 2031360302985516,
        // Sith Warrior Styles
        Guardian = 2484207912698169,
        Juggernaut = 2205476972965177,
        // Sith Inquisitor Styles
        Shadow = 3008608613884233,
        Assassin = 2031354002985098
	};

    enum class CombatRole : uint8_t {
        Unknown = 0,
        DPS,
        Healer,
        Tank
	};

    // Disciplines/Specializations for each class
    enum class Discipline : uint64_t {
        Unknown = 0,

        // Trooper/Bounty Hunter (Commando/Mercenary and Vanguard/Powertech)
        Gunnery = 3088803483451154,            // Commando DPS
        CombatMedic = 1610854127306954,        // Commando Healer
        AssaultSpecialist = 3739871355530330,  // Commando DPS
        Arsenal = 594992886408418,             // Mercenary DPS
        Bodyguard = 2203256920318106,          // Mercenary Healer
        InnovativeOrdnance = 3507396390530202, // Mercenary DPS
        Tactics = 1944502563654938,            // Vanguard DPS
        ShieldSpecialist = 3007101716805754,   // Vanguard Tank
        Plasmatech = 1944487867571386,         // Vanguard DPS
        AdvancedPrototype = 3320456030634170,  // Powertech DPS
        ShieldTech = 1929098417348794,         // Powertech Tank
        Pyrotech = 3320419469872442,           // Powertech DPS

        // Smuggler/Imperial Agent (Gunslinger/Sniper and Scoundrel/Operative)
        Sharpshooter = 3508869182982330,       // Gunslinger DPS
        Saboteur = 3322083181395130,           // Gunslinger DPS
        Dirty = 1946011866315962,              // Gunslinger DPS
        Marksmanship = 3225114604527898,       // Sniper DPS
        Engineering = 2031374702903449,        // Sniper DPS
        Virulence = 3109089216887066,          // Sniper DPS
        Scrapper = 2487504318513466,           // Scoundrel DPS
        Sawbones = 2487567242063162,           // Scoundrel Healer
        Ruffian = 2485828043867450,            // Scoundrel DPS
        Concealment = 2031360302985517,        // Operative DPS
        Lethality = 2031339142381593,          // Operative DPS
        Medicine = 1932232264187162,           // Operative Healer

        // Jedi Knight/Sith Warrior (Sentinel/Marauder and Guardian/Juggernaut)
        Watchman = 3508879977426106,           // Sentinel DPS
        Combat = 3218632854835386,             // Sentinel DPS
        Concentration = 3218654353789114,      // Sentinel DPS
        Annihilation = 3219155620896954,       // Marauder DPS
        Carnage = 3219159918885050,            // Marauder DPS
        Fury = 595034142806330,                // Marauder DPS
        Vigilance = 2484207912698170,          // Guardian DPS
        Defense = 1929098417479866,            // Guardian Tank
        Focus = 1944538822886714,              // Guardian DPS
        Vengeance = 2205476972965178,          // Juggernaut DPS
        Immortal = 1913582031199546,           // Juggernaut Tank
        Rage = 3300945127303354,               // Juggernaut DPS

        // Jedi Consular/Sith Inquisitor (Sage/Sorcerer and Shadow/Assassin)
        Telekinetics = 1944553467445562,       // Sage DPS
        Balance = 3219158918873786,            // Sage DPS
        Seer = 3218621659655354,               // Sage Healer
        Lightning = 3300941827327162,          // Sorcerer DPS
        Madness = 2487654488367418,            // Sorcerer DPS
        Corruption = 583093866373434,          // Sorcerer Healer
        Infiltration = 3008608613884234,       // Shadow DPS
        Serenity = 3219148223905914,           // Shadow DPS
        KineticCombat = 3218586805260602,      // Shadow Tank (name in game: "Kinetic Combat")
        Deception = 2031354002985099,          // Assassin DPS
        Hatred = 2487472243868986,             // Assassin DPS
        Darkness = 1930851419333946            // Assassin Tank
    };

    // Helper to get class name
    const char* combat_class_name(CombatClass cls);

    // Helper to get discipline name
    const char* discipline_name(Discipline disc);

    // ---------- Identity & core data ----------
    enum class EntityKind : uint8_t { Unknown = 0, Player, NpcOrObject };

    struct EntityId {
        uint64_t hi{ 0 }, lo{ 0 };
        EntityKind kind{ EntityKind::Unknown };
        bool operator==(const EntityId& o) const { return hi == o.hi && lo == o.lo && kind == o.kind; }
        bool valid() const { return kind != EntityKind::Unknown && (hi || lo); }
    };

    struct Health { int64_t current{ 0 }; int64_t max{ 0 }; };
    struct Position 
    { float x{ 0 }, y{ 0 }, z{ 0 }, facing{ 0 }; 
        
    };

    struct CompanionOwner {
        std::string      name_no_at{};
        uint64_t player_numeric_id{ 0 };
        bool has_owner{ false };
    };

    struct Entity {
        std::string      display{};
        std::string      name{};
        std::string      companion_name{};
        EntityId id{};
        bool is_player{ false };
        bool is_companion{ false };
        bool empty{ false };
        bool is_same_as_source{ false };
        Position pos{};
        Health   hp{};
        CompanionOwner owner{};
    };

    struct NamedId {
        std::string      name{};
        uint64_t id{ 0 };
    };

    struct EventEffect {

        /// <summary>
		/// The numeric Type ID if available, otherwise 0. Such as 836045448945477 or ApplyEffect. or 
        /// </summary>
        uint64_t  type_id{ 0 };

        /// <summary>
		/// Action ID is the numeric Action ID if available, otherwise 0. It will reflect the ability or ability-like action.
        /// </summary>
        uint64_t  action_id{ 0 };

        /// <summary>
		/// Name of the event type, such as "ApplyEffect"
        /// </summary>
        std::string type_name{};
        /// <summary>
		/// Name of the action/ability, such as "Corrosive Grenade"
        /// </summary>
        std::string action_name{};


        /// <summary>
		/// Store full event data string for special parsing later (such as AreaEntered)
        /// </summary>
        std::string data{};

        bool matches(const EventType& et) const;

        bool matches(const uint64_t id) const;

        bool matches(const EventActionType& eat) const;

    };


    inline bool operator==(const EventEffect& p, const EventType& et) { return p.matches(et); }

    inline bool operator==(const EventEffect& p, const EventActionType& eat) { return p.matches(eat); }

    inline bool operator==(const EventEffect& p, const uint64_t id) { return p.matches(id); }


    struct TimeStamp {
        uint32_t combat_ms{ 0 };          // HH:MM:SS.mmm → ms since midnight
        int64_t  refined_epoch_ms{ -1 };  // fill later with NTP/refinement
    };

    // ---------- Event-specific data structures ----------

    // Data specific to AreaEntered events
    struct AreaEnteredData {
        NamedId area{};                    // Area name and ID (e.g., "Nar Shaddaa")
        NamedId difficulty{};              // Optional difficulty (e.g., "8 Player Master")
        std::string version{};             // Version tag (e.g., "v7.0.0b")
        std::string raw_value{};           // Raw value field (e.g., "he3001")
        bool has_difficulty{ false };
    };

    // Data specific to DisciplineChanged events
    struct DisciplineChangedData {
        NamedId combat_class{};            // Combat class (e.g., "Operative")
        NamedId discipline{};              // Discipline/spec (e.g., "Lethality")
        CombatClass combat_class_enum{ CombatClass::Unknown };
        Discipline discipline_enum{ Discipline::Unknown };
		CombatRole role_enum{ CombatRole::Unknown };
    };

    // ---------- Trailing (value/mitigation/threat) ----------
    enum class MitigationFlags : uint16_t {
        None = 0,
        Shield = 1 << 0,
        Deflect = 1 << 1,
        Glance = 1 << 2,
        Dodge = 1 << 3,
        Parry = 1 << 4,
        Resist = 1 << 5,
        Miss = 1 << 6,
        Immune = 1 << 7,
    };
    inline constexpr MitigationFlags operator|(MitigationFlags a, MitigationFlags b) {
        return static_cast<MitigationFlags>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
    }
    inline constexpr MitigationFlags& operator|=(MitigationFlags& a, MitigationFlags b) { a = a | b; return a; }

    enum class ValueKind : uint8_t { None = 0, Numeric, Charges, Unknown };

    struct School {
        std::string      name{};
        uint64_t id{ 0 };
        bool present{ false };
    };

    struct ShieldDetail {
        uint64_t shield_effect_id{ 0 };
        int64_t  absorbed{ 0 };
        uint64_t absorbed_id{ 0 };
        bool present{ false };
    };
    struct ValueField {
        int64_t amount{ 0 };
        bool    crit{ false };
        bool    has_secondary{ false };
        int64_t secondary{ 0 };
        School  school{};
        MitigationFlags mitig{ MitigationFlags::None };
        ShieldDetail shield{};
    };

    struct Trailing {
        ValueKind kind{ ValueKind::None };
        ValueField val{};
        int32_t charges{ 0 };
        bool has_charges{ false };
        bool has_threat{ false };
        double threat{ 0.0 };             // accepts <123.0> etc.
        std::string_view unparsed{};    // leftover tail if any
    };

    // ---------- Whole line ----------
    enum class ParseStatus : uint8_t { Ok = 0, Malformed };

    struct CombatLine {
        TimeStamp t{};
        Entity source{};
        Entity target{};
        NamedId   ability{};
        EventEffect event{};
        Trailing tail{};

        // Event-specific data (populated based on evt.kind)
        AreaEnteredData area_entered{};
        DisciplineChangedData discipline_changed{};

    };


    inline bool operator==(const CombatLine& p, const EventType& et) { return p.event.matches(et); }


    // Parse a single SWTOR combat log line into CombatLine (allocation-free).
    ParseStatus parse_combat_line(std::string_view line, CombatLine& out);

	extern CombatRole deduce_combat_role(Discipline disc);

    // ---------- Pretty printer ----------
    struct PrintOptions {
        bool multiline = true;
        bool include_positions = false;
        bool include_health = false;
    };

    // Text (human) pretty-print for quick debugging/logging.
    std::string to_text(const CombatLine& L, const PrintOptions& opt = {});

    // ---------- JSON round-trip ----------
    namespace detail_json {
        // Simple arena that owns backing storage for string_views when parsing JSON or deep-cloning.
        struct StringArena {
            std::string buf;
            std::string_view intern(std::string_view s) {
                const size_t off = buf.size();
                buf.append(s.data(), s.size());
                return std::string_view(buf.data() + off, s.size());
            }
        };
    } // namespace detail_json

    // Serialize a CombatLine to compact JSON (schema-locked).
    std::string to_json(const CombatLine& L);

    // Parse JSON produced by to_json() back into CombatLine.
    // String data is stored in 'arena' so string_views remain valid.
    bool from_json(std::string_view json, CombatLine& out, detail_json::StringArena& arena);

    // ---------- Event filtering sugar ----------
    // ---------- Ownership helpers (clone & dispose) ----------
    // Intern every string_view from src into arena, write into dst.
    void deep_bind_into(CombatLine& dst, const CombatLine& src, detail_json::StringArena& arena);

    // A self-contained, deep-copied CombatLine. All string_views point into 'arena'.
    struct OwnedCombatLine {
        CombatLine value;
        detail_json::StringArena arena;

        OwnedCombatLine() = default;
        explicit OwnedCombatLine(const CombatLine& src) { deep_bind_into(value, src, arena); }

        // Deep copy
        OwnedCombatLine(const OwnedCombatLine& other) { deep_bind_into(value, other.value, arena); }
        OwnedCombatLine& operator=(const OwnedCombatLine& other) {
            if (this != &other) {
                arena.buf.clear();
                deep_bind_into(value, other.value, arena);
            }
            return *this;
        }

        // Moves are cheap
        OwnedCombatLine(OwnedCombatLine&&) noexcept = default;
        OwnedCombatLine& operator=(OwnedCombatLine&&) noexcept = default;

        // Dispose (release memory; value's views become empty)
        void reset() {
            value = CombatLine{};
            arena.buf.clear();
            arena.buf.shrink_to_fit(); // optional
        }
    };

    // Convenience free function
    OwnedCombatLine make_owned_clone(const CombatLine& src);

    // Pool that owns one big arena; cloned lines share the same backing store.
    struct OwnedLinePool {
        detail_json::StringArena arena;

        CombatLine clone_into(const CombatLine& src) {
            CombatLine dst{};
            deep_bind_into(dst, src, arena);
            return dst;
        }
        void clear() {
            arena.buf.clear();
            arena.buf.shrink_to_fit();
        }
    };

    // ---------- Small helpers (exposed for convenience) ----------
    std::string flags_to_string(MitigationFlags f);

    inline bool parse_named_id(std::string_view text, NamedId& out);

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
            AreaEnteredData& out);

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
            DisciplineChangedData& out);

        // Detect event kind from event type name

    } // namespace detail


    static inline void parse_mitigation_tail(std::string_view rest, ValueField& vf);
    // Helper function to check if a CombatLine is an AreaEntered event
    inline bool is_area_entered(const CombatLine& line) {
		return line.event == swtor::EventType::AreaEntered;
    }

    // Helper function to check if a CombatLine is a DisciplineChanged event
    inline bool is_discipline_changed(const CombatLine& line) {
		return line.event == swtor::EventType::DisciplineChanged;
    }

    // Get area name from AreaEntered event (returns empty string if not an AreaEntered event)
    inline std::string_view get_area_name(const CombatLine& line) {
        if (line.event == EventType::AreaEntered) {
            return line.area_entered.area.name;
        }
        return "";
    }

    // Get difficulty name from AreaEntered event (returns empty string if not present)
    inline std::string_view get_difficulty_name(const CombatLine& line) {
        if (line.event == EventType::AreaEntered && line.area_entered.has_difficulty) {
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
