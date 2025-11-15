#pragma once
#include <cstdint>
#include <string>
#include <string_view>

namespace swtor {

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
    struct Position { float x{ 0 }, y{ 0 }, z{ 0 }, facing{ 0 }; };

    struct CompanionOwner {
        std::string      name_no_at{};
        uint64_t player_numeric_id{ 0 };
        bool has_owner{ false };
    };

    struct EntityRef {
        std::string      display{};
        std::string      name{};
        std::string      companion_name{};
        EntityId id{};
        bool is_player{ false };
        bool is_companion{ false };
        bool empty{ false };
        Position pos{};
        Health   hp{};
        CompanionOwner owner{};
    };

    struct NamedId {
        std::string      name{};
        uint64_t id{ 0 };
    };

    enum class EventKind : uint8_t {
        Unknown = 0, 
        ApplyEffect, 
        RemoveEffect, 
        Event, 
        Spend, 
        Restore, 
        ModifyCharges,
        AreaEntered,        // Special event for zone changes
        DisciplineChanged   // Special event for discipline/loadout changes
    };

    struct EventEffect {
        EventKind kind{ EventKind::Unknown };
        uint64_t  kind_id{ 0 };
        NamedId   effect{};
    };

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
        EntityRef source{};
        EntityRef target{};
        NamedId   ability{};
        EventEffect evt{};
        Trailing tail{};
        
        // Event-specific data (populated based on evt.kind)
        AreaEnteredData area_entered{};
        DisciplineChangedData discipline_changed{};
    };

    // Parse a single SWTOR combat log line into CombatLine (allocation-free).
    ParseStatus parse_combat_line(std::string_view line, CombatLine& out);

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
    struct EventTag {
        EventKind k;
        constexpr explicit EventTag(EventKind kk) : k(kk) {}
    };
    inline constexpr uint32_t event_bit(EventKind k) { return 1u << static_cast<unsigned>(k); }

    struct EventSet {
        uint32_t mask{ 0 };
        constexpr bool matches(const CombatLine& L) const {
            return (mask & event_bit(L.evt.kind)) != 0u;
        }
    };
    inline constexpr bool operator==(const EventTag& tag, const CombatLine& L) { return L.evt.kind == tag.k; }
    inline constexpr bool operator==(const CombatLine& L, const EventTag& tag) { return tag == L; }
    inline constexpr EventSet operator|(EventTag a, EventTag b) { return { event_bit(a.k) | event_bit(b.k) }; }
    inline constexpr EventSet operator|(EventSet s, EventTag a) { return { s.mask | event_bit(a.k) }; }
    inline constexpr bool operator==(const EventSet& set, const CombatLine& L) { return set.matches(L); }

    namespace Events {
        inline constexpr EventTag ApplyEffect{ EventKind::ApplyEffect };
        inline constexpr EventTag RemoveEffect{ EventKind::RemoveEffect };
        inline constexpr EventTag Event{ EventKind::Event };
        inline constexpr EventTag Spend{ EventKind::Spend };
        inline constexpr EventTag Restore{ EventKind::Restore };
        inline constexpr EventTag ModifyCharges{ EventKind::ModifyCharges };
        inline constexpr EventTag AreaEntered{ EventKind::AreaEntered };
        inline constexpr EventTag DisciplineChanged{ EventKind::DisciplineChanged };

        // bundles
        inline constexpr EventSet Effects = ApplyEffect | RemoveEffect;
    }

    // ---------- EventPred (more flexible predicates) ----------

    struct EventPred {
        // What to match:
        EventKind kind{ EventKind::Unknown };
        bool has_kind{ false };

        std::string_view effect_name{};
        bool has_effect_name{ false };

        uint64_t effect_id{ 0 };
        bool has_effect_id{ false };

        // Factory helpers
        static constexpr EventPred Kind(EventKind k) {
            EventPred p; p.kind = k; p.has_kind = true; return p;
        }
        template <size_t N>
        static constexpr EventPred Effect(const char(&lit)[N]) {
            EventPred p; p.effect_name = std::string_view(lit, N - 1); p.has_effect_name = true; return p;
        }
        template <size_t N>
        static constexpr EventPred KindEffect(EventKind k, const char(&lit)[N]) {
            EventPred p; p.kind = k; p.has_kind = true;
            p.effect_name = std::string_view(lit, N - 1); p.has_effect_name = true; return p;
        }
        static constexpr EventPred EffectId(uint64_t id) {
            EventPred p; p.effect_id = id; p.has_effect_id = true; return p;
        }

        // Runtime match
        bool matches(const CombatLine& L) const;
    };

    // OR-combination of predicates (small fixed-size array to avoid allocs)
    struct AnyPred {
        EventPred items[8];
        uint8_t   count{ 0 };

        bool matches(const CombatLine& L) const {
            for (uint8_t i = 0; i < count; i++) if (items[i].matches(L)) return true;
            return false;
        }
    };

    inline bool operator==(const EventPred& p, const CombatLine& L) { return p.matches(L); }
    inline bool operator==(const CombatLine& L, const EventPred& p) { return p.matches(L); }

    inline AnyPred operator|(const EventPred& a, const EventPred& b) {
        AnyPred ap; ap.items[0] = a; ap.items[1] = b; ap.count = 2; return ap;
    }
    inline AnyPred operator|(AnyPred ap, const EventPred& b) {
        if (ap.count < 8) ap.items[ap.count++] = b;
        return ap;
    }
    inline bool operator==(const AnyPred& ap, const CombatLine& L) { return ap.matches(L); }
    inline bool operator==(const CombatLine& L, const AnyPred& ap) { return ap.matches(L); }

    // Convenience tokens
    namespace Evt {
        // Kinds
        inline constexpr EventPred ApplyEffect = EventPred::Kind(EventKind::ApplyEffect);
        inline constexpr EventPred RemoveEffect = EventPred::Kind(EventKind::RemoveEffect);
        inline constexpr EventPred Ev = EventPred::Kind(EventKind::Event);
        inline constexpr EventPred Spend = EventPred::Kind(EventKind::Spend);
        inline constexpr EventPred Restore = EventPred::Kind(EventKind::Restore);
        inline constexpr EventPred ModifyCharges = EventPred::Kind(EventKind::ModifyCharges);
        inline constexpr EventPred AreaEntered = EventPred::Kind(EventKind::AreaEntered);
        inline constexpr EventPred DisciplineChanged = EventPred::Kind(EventKind::DisciplineChanged);

        // Common effect names (change/add more as needed)
        inline constexpr EventPred Damage = EventPred::Effect("Damage");
        inline constexpr EventPred Heal = EventPred::Effect("Heal");
        inline constexpr EventPred Taunt = EventPred::Effect("Taunt");
        inline constexpr EventPred Threat = EventPred::Effect("Threat");

        // Kind + effect combos
        inline constexpr EventPred EventDamage = EventPred::KindEffect(EventKind::Event, "Damage");
        inline constexpr EventPred EventHeal = EventPred::KindEffect(EventKind::Event, "Heal");
    }

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
    const char* kind_to_cstr(EventKind k);
    std::string flags_to_string(MitigationFlags f);

} // namespace swtor
