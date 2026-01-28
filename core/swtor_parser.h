#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <iostream>
#include <format>
#include <locale>

namespace swtor {




    /// <summary>
    /// AreaEntered event type identifier
    /// </summary>
    inline constexpr uint64_t KINDID_AreaEntered = 836045448953664ULL;
    /// <summary>
    /// DisciplineChanged event type identifier
    /// </summary>
    inline constexpr uint64_t KINDID_DisciplineChanged = 836045448953665ULL;

    /// <summary>
    /// Event type identifier
    /// </summary>
    inline constexpr uint64_t KINDID_Event = 836045448945472ULL;
    /// <summary>
    /// Spend event type identifier
    /// </summary>
    inline constexpr uint64_t KINDID_Spend = 836045448945473ULL;
    /// <summary>
    /// Restore event type identifier
    /// </summary>
    inline constexpr uint64_t KINDID_Restore = 836045448945476ULL;
    /// <summary>
    /// ApplyEffect event type identifier
    /// </summary>
    inline constexpr uint64_t KINDID_ApplyEffect = 836045448945477ULL;
    /// <summary>
    /// RemoveEffect event type identifier
    /// </summary>
    inline constexpr uint64_t KINDID_RemoveEffect = 836045448945478ULL;
    /// <summary>
    /// ModifyCharges event type identifier
    /// </summary>
    inline constexpr uint64_t KINDID_ModifyCharges = 836045448953666ULL;

    /// <summary>
    /// Combat classes (8 original classes mapped to Combat Styles in 7.0+)
    /// </summary>
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

    /// <summary>
    /// Area difficulty levels for operations and flashpoints
    /// </summary>
    enum class AreaDifficulty : uint64_t {
        Unknown = 0,
        Solo = 1,
		Story_4 = 836045448953656,
		Veteran_4 = 836045448953657,
		Master_4 = 836045448953659,
		Story_8 = 836045448953651,
		Veteran_8 = 836045448953652,
        Master_8 = 836045448953655,
		Story_16 = 836045448953653,
        Veteran_16 = 836045448953654,
		Master_16 = 836045448953658
    };

    /// <summary>
    /// Deduce area difficulty from difficulty ID
    /// </summary>
    /// <param name="difficult_id">Difficulty identifier</param>
    /// <returns>Area difficulty enum value</returns>
    inline AreaDifficulty deduce_area_difficulty(uint64_t difficult_id) {
		AreaDifficulty result = AreaDifficulty::Solo;
        return result;
    }

    /// <summary>
    /// Get the number of players for a given difficulty level
    /// </summary>
    /// <param name="diff">Area difficulty</param>
    /// <returns>Number of players (1, 4, 8, or 16)</returns>
    inline int number_of_players(AreaDifficulty diff) {
        switch (diff) {
            case AreaDifficulty::Solo:
                return 1;
            case AreaDifficulty::Story_8:
            case AreaDifficulty::Veteran_8:
            case AreaDifficulty::Master_8:
                return 8;
            case AreaDifficulty::Story_16:
            case AreaDifficulty::Veteran_16:
            case AreaDifficulty::Master_16:
                return 16;
            default:
                return 0;
        }
	}

    /// <summary>
    /// Event types that can occur in combat logs
    /// </summary>
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

    /// <summary>
    /// Specific action types within events
    /// </summary>
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

    /// <summary>
    /// Combat styles available to characters
    /// </summary>
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

    /// <summary>
    /// Combat role classifications
    /// </summary>
    enum class CombatRole : uint8_t {
        Unknown = 0,
        DPS,
        Healer,
        Tank
	};

    /// <summary>
    /// Disciplines/Specializations for each class
    /// </summary>
    enum class Discipline : uint64_t {
        Unknown = 0,

        // Trooper/Bounty Hunter (Commando/Mercenary and Vanguard/Powertech)
        Gunnery = 3088803483451154,
        CombatMedic = 1610854127306954,
        AssaultSpecialist = 3739871355530330,
        Arsenal = 594992886408418,
        Bodyguard = 2203256920318106,
        InnovativeOrdnance = 3507396390530202,
        Tactics = 1944502563654938,
        ShieldSpecialist = 3007101716805754,
        Plasmatech = 1944487867571386,
        AdvancedPrototype = 3320456030634170,
        ShieldTech = 1929098417348794,
        Pyrotech = 3320419469872442,

        // Smuggler/Imperial Agent (Gunslinger/Sniper and Scoundrel/Operative)
        Sharpshooter = 3508869182982330,
        Saboteur = 3322083181395130,
        Dirty = 1946011866315962,
        Marksmanship = 3225114604527898,
        Engineering = 2031374702903449,
        Virulence = 3109089216887066,
        Scrapper = 2487504318513466,
        Sawbones = 2487567242063162,
        Ruffian = 2485828043867450,
        Concealment = 2031360302985517,
        Lethality = 2031339142381593,
        Medicine = 1932232264187162,

        // Jedi Knight/Sith Warrior (Sentinel/Marauder and Guardian/Juggernaut)
        Watchman = 3508879977426106,
        Combat = 3218632854835386,
        Concentration = 3218654353789114,
        Annihilation = 3219155620896954,
        Carnage = 3219159918885050,
        Fury = 595034142806330,
        Vigilance = 2484207912698170,
        Defense = 1929098417479866,
        Focus = 1944538822886714,
        Vengeance = 2205476972965178,
        Immortal = 1913582031199546,
        Rage = 3300945127303354,

        // Jedi Consular/Sith Inquisitor (Sage/Sorcerer and Shadow/Assassin)
        Telekinetics = 1944553467445562,
        Balance = 3219158918873786,
        Seer = 3218621659655354,
        Lightning = 3300941827327162,
        Madness = 2487654488367418,
        Corruption = 583093866373434,
        Infiltration = 3008608613884234,
        Serenity = 3219148223905914,
        KineticCombat = 3218586805260602,
        Deception = 2031354002985099,
        Hatred = 2487472243868986,
        Darkness = 1930851419333946
    };

    /// <summary>
    /// Get the combat class name as a string
    /// </summary>
    /// <param name="cls">Combat class enum value</param>
    /// <returns>Combat class name</returns>
    const char* combat_class_name(CombatClass cls);

    /// <summary>
    /// Get the discipline name as a string
    /// </summary>
    /// <param name="disc">Discipline enum value</param>
    /// <returns>Discipline name</returns>
    const char* discipline_name(Discipline disc);

    /// <summary>
    /// Entity kind classification
    /// </summary>
    enum class EntityKind : uint8_t { Unknown = 0, Player, NpcOrObject };

    /// <summary>
    /// Entity health status
    /// </summary>
    struct Health { 
        /// <summary>
        /// Current hit points
        /// </summary>
        int64_t current{ 0 }; 
        /// <summary>
        /// Maximum hit points
        /// </summary>
        int64_t max{ 0 }; 
    };
    
    /// <summary>
    /// Entity position and facing
    /// </summary>
    struct Position 
    { 
        /// <summary>
        /// X coordinate
        /// </summary>
        float x{ 0 }; 
        /// <summary>
        /// Y coordinate
        /// </summary>
        float y{ 0 }; 
        /// <summary>
        /// Z coordinate
        /// </summary>
        float z{ 0 }; 
        /// <summary>
        /// Facing direction
        /// </summary>
        float facing{ 0 }; 
        
    };

    /// <summary>
    /// Companion ownership information
    /// </summary>
    struct CompanionOwner {
        /// <summary>
        /// Owner name without @ symbol
        /// </summary>
        std::string      name_no_at{};
        /// <summary>
        /// Owner player numeric ID
        /// </summary>
        uint64_t player_numeric_id{ 0 };
        /// <summary>
        /// Whether companion has a valid owner
        /// </summary>
        bool has_owner{ false };
    };

    /// <summary>
    /// Entity representation (player, NPC, companion, or object)
    /// </summary>
    struct Entity {
        /// <summary>
        /// Display name with formatting
        /// </summary>
        std::string      display{};
        /// <summary>
        /// Base name
        /// </summary>
        std::string      name{};
        /// <summary>
        /// Companion name if applicable
        /// </summary>
        std::string      companion_name{};
        /// <summary>
        /// Entity unique identifier
        /// </summary>
        uint64_t id{};
        /// <summary>
        /// Entity type identifier
        /// </summary>
        uint64_t type_id{};
        /// <summary>
        /// Whether entity is a player
        /// </summary>
        bool is_player{ false };
        /// <summary>
        /// Whether entity is a companion
        /// </summary>
        bool is_companion{ false };
        /// <summary>
        /// Whether entity data is empty
        /// </summary>
        bool empty{ false };
        /// <summary>
        /// Whether entity is the same as the source entity
        /// </summary>
        bool is_same_as_source{ false };
        /// <summary>
        /// Entity position
        /// </summary>
        Position pos{};
        /// <summary>
        /// Entity health
        /// </summary>
        Health   hp{};
        /// <summary>
        /// Companion owner information
        /// </summary>
        CompanionOwner owner{};

        std::shared_ptr<Entity> get_shared() const {
            return std::make_shared<Entity>(*this);  // Copy construct
        }
    };

    /// <summary>
    /// Compare two entities by ID
    /// </summary>
    /// <param name="p1">First entity</param>
    /// <param name="p2">Second entity</param>
    /// <returns>True if IDs match</returns>
    inline bool operator==(const Entity& p1, const Entity& p2) { return p1.id == p2.id; }

    inline bool operator==(const std::shared_ptr<Entity> p1, const Entity& p2) {
        if (!p1) return false;
        return p1->id == p2.id;
    }
    inline bool operator==(const Entity& p2, const std::shared_ptr<Entity> p1) {
        if (!p1) return false;
        return p1->id == p2.id;
    }

    inline bool operator==(const std::shared_ptr<Entity> p1, const std::shared_ptr<Entity> p2) {
        if (!p1 && !p2) return true;
        if (!p1) return false;
        if (!p2) return false;
        return p1->id == p2->id;
    }
    /// <summary>
    /// Named identifier with numeric ID
    /// </summary>
    struct NamedId {
        /// <summary>
        /// Name string
        /// </summary>
        std::string      name{};
        /// <summary>
        /// Numeric identifier
        /// </summary>
        uint64_t id{ 0 };
    };

    /// <summary>
    /// Event effect data
    /// </summary>
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

        /// <summary>
        /// Check if event matches an EventType
        /// </summary>
        /// <param name="et">Event type to match</param>
        /// <returns>True if matches</returns>
        bool matches(const EventType& et) const;

        /// <summary>
        /// Check if event matches a numeric ID
        /// </summary>
        /// <param name="id">ID to match</param>
        /// <returns>True if matches</returns>
        bool matches(const uint64_t id) const;

        /// <summary>
        /// Check if event matches an EventActionType
        /// </summary>
        /// <param name="eat">Event action type to match</param>
        /// <returns>True if matches</returns>
        bool matches(const EventActionType& eat) const;

    };


    /// <summary>
    /// Compare event effect with event type
    /// </summary>
    /// <param name="p">Event effect</param>
    /// <param name="et">Event type</param>
    /// <returns>True if matches</returns>
    inline bool operator==(const EventEffect& p, const EventType& et) { return p.matches(et); }

    /// <summary>
    /// Compare event effect with event action type
    /// </summary>
    /// <param name="p">Event effect</param>
    /// <param name="eat">Event action type</param>
    /// <returns>True if matches</returns>
    inline bool operator==(const EventEffect& p, const EventActionType& eat) { return p.matches(eat); }

    /// <summary>
    /// Compare event effect with numeric ID
    /// </summary>
    /// <param name="p">Event effect</param>
    /// <param name="id">Numeric ID</param>
    /// <returns>True if matches</returns>
    inline bool operator==(const EventEffect& p, const uint64_t id) { return p.matches(id); }


    /// <summary>
    /// Timestamp information from combat log
    /// </summary>
    struct TimeStamp {
        /// <summary>
        /// HH:MM:SS.mmm → ms since midnight
        /// </summary>
        uint32_t combat_ms{ 0 };
        /// <summary>
        /// Refined epoch timestamp in milliseconds (filled by time cruncher)
        /// </summary>
        int64_t  refined_epoch_ms{ -1 };
        /// <summary>
        /// hour
        /// </summary>
        uint32_t h{ 0 };
        /// <summary>
        /// minute
        /// </summary>
        uint32_t m{ 0 };
        /// <summary>
        /// seconds
        /// </summary>
        uint32_t s{ 0 };
        /// <summary>
        /// milliseconds
        /// </summary>
        uint32_t ms{ 0 };
        /// <summary>
        /// Year
        /// </summary>
        uint32_t year{ 0 };
        /// <summary> 
        /// Month
        /// </summary>
        uint32_t month{ 0 };
        /// <summary>
        /// Day
        /// </summary>
        uint32_t day{ 0 };

        /// <summary>
        /// Convert refined epoch milliseconds to time point
        /// </summary>
        /// <returns>System clock time point</returns>
        inline std::chrono::system_clock::time_point to_time_point() const {
            using namespace std::chrono;
            return system_clock::time_point(milliseconds(refined_epoch_ms));
		}

        /// <summary>
        /// Set refined epoch milliseconds from time point
        /// </summary>
        /// <param name="tp">System clock time point</param>
        inline void from_time_point(const std::chrono::system_clock::time_point& tp) {
            using namespace std::chrono;
            refined_epoch_ms = duration_cast<milliseconds>(tp.time_since_epoch()).count();
		}

        /// <summary>
        /// Updates the combat_ms value from the h, m, s, and ms values
        /// </summary>
        inline void update_combat_ms() {
            combat_ms = uint32_t(((h * 60 + m) * 60 + s) * 1000 + ms);
        }

        /// <summary>
        /// Format timestamp as printable string
        /// </summary>
        /// <returns>Formatted timestamp string</returns>
        inline std::string print() const {
            std::ostringstream oss;
            if (year > 0) {
                oss << std::setw(4) << std::setfill('0') << year;
                oss << "-" << std::setw(2) << std::setfill('0') << month;
                oss << "-" << std::setw(2) << std::setfill('0') << day;
                oss << " ";
            }
            oss << std::setw(2) << std::setfill('0') << h;
            oss << ":" << std::setw(2) << std::setfill('0') << m;
            oss << ":" << std::setw(2) << std::setfill('0') << s;
            oss << "." << std::setw(3) << std::setfill('0') << ms;
            return oss.str();
        }
    };

    /// <summary>
    /// Data specific to AreaEntered events
    /// </summary>
    struct AreaEnteredData {
        /// <summary>
        /// Area name and ID
        /// </summary>
        NamedId area{};
        /// <summary>
        /// Optional difficulty name and ID
        /// </summary>
        NamedId difficulty{};
		/// <summary>
		/// Parsed difficulty enum value
		/// </summary>
		AreaDifficulty difficulty_value{ AreaDifficulty::Unknown };
        /// <summary>
        /// Game version tag (e.g., "v7.0.0b")
        /// </summary>
        std::string version{};
        /// <summary>
        /// Raw value field from log
        /// </summary>
        std::string raw_value{};
        /// <summary>
        /// Whether difficulty information is present
        /// </summary>
        bool has_difficulty{ false };
    };

    /// <summary>
    /// Data specific to DisciplineChanged events
    /// </summary>
    struct DisciplineChangedData {
        /// <summary>
        /// Combat class name and ID
        /// </summary>
        NamedId combat_class{};
        /// <summary>
        /// Discipline/spec name and ID
        /// </summary>
        NamedId discipline{};
        /// <summary>
        /// Parsed combat class enum value
        /// </summary>
        CombatClass combat_class_enum{ CombatClass::Unknown };
        /// <summary>
        /// Parsed discipline enum value
        /// </summary>
        Discipline discipline_enum{ Discipline::Unknown };
		/// <summary>
		/// Deduced combat role
		/// </summary>
		CombatRole role_enum{ CombatRole::Unknown };
    };

    /// <summary>
    /// Mitigation flags for damage reduction/avoidance
    /// </summary>
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
    /// <summary>
    /// Bitwise OR operator for mitigation flags
    /// </summary>
    /// <param name="a">First flag</param>
    /// <param name="b">Second flag</param>
    /// <returns>Combined flags</returns>
    inline constexpr MitigationFlags operator|(MitigationFlags a, MitigationFlags b) {
        return static_cast<MitigationFlags>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
    }
    /// <summary>
    /// Bitwise OR assignment operator for mitigation flags
    /// </summary>
    /// <param name="a">First flag (modified)</param>
    /// <param name="b">Second flag</param>
    /// <returns>Combined flags</returns>
    inline constexpr MitigationFlags& operator|=(MitigationFlags& a, MitigationFlags b) { a = a | b; return a; }

    /// <summary>
    /// Value kind classification
    /// </summary>
    enum class ValueKind : uint8_t { None = 0, Numeric, Charges, Unknown };

    /// <summary>
    /// Damage school information
    /// </summary>
    struct School {
        /// <summary>
        /// School name
        /// </summary>
        std::string      name{};
        /// <summary>
        /// School numeric ID
        /// </summary>
        uint64_t id{ 0 };
        /// <summary>
        /// Whether school information is present
        /// </summary>
        bool present{ false };
    };

    /// <summary>
    /// Shield absorption details
    /// </summary>
    struct ShieldDetail {
        /// <summary>
        /// Shield effect ID
        /// </summary>
        uint64_t shield_effect_id{ 0 };
        /// <summary>
        /// Amount absorbed by shield
        /// </summary>
        int64_t  absorbed{ 0 };
        /// <summary>
        /// Absorbed ability ID
        /// </summary>
        uint64_t absorbed_id{ 0 };
        /// <summary>
        /// Whether shield detail is present
        /// </summary>
        bool present{ false };
    };
    
    /// <summary>
    /// Value field containing damage/healing amount and modifiers
    /// </summary>
    struct ValueField {
        /// <summary>
        /// Damage or healing amount
        /// </summary>
        int64_t amount{ 0 };
        /// <summary>
        /// Whether this was a critical hit/heal
        /// </summary>
        bool    crit{ false };
        /// <summary>
        /// Whether secondary value is present
        /// </summary>
        bool    has_secondary{ false };
        /// <summary>
        /// Secondary value (e.g., overheal)
        /// </summary>
        int64_t secondary{ 0 };
        /// <summary>
        /// Damage school
        /// </summary>
        School  school{};
        /// <summary>
        /// Mitigation flags
        /// </summary>
        MitigationFlags mitig{ MitigationFlags::None };
        /// <summary>
        /// Shield absorption details
        /// </summary>
        ShieldDetail shield{};
    };

    /// <summary>
    /// Trailing data at end of combat line (values, charges, threat)
    /// </summary>
    struct Trailing {
        /// <summary>
        /// Kind of value present
        /// </summary>
        ValueKind kind{ ValueKind::None };
        /// <summary>
        /// Value field data
        /// </summary>
        ValueField val{};
        /// <summary>
        /// Number of charges/stacks
        /// </summary>
        int32_t charges{ 0 };
        /// <summary>
        /// Whether charges are present
        /// </summary>
        bool has_charges{ false };
        /// <summary>
        /// Whether threat value is present
        /// </summary>
        bool has_threat{ false };
        /// <summary>
        /// Threat value
        /// </summary>
        double threat{ 0.0 };
        /// <summary>
        /// Unparsed remainder of trailing data
        /// </summary>
        std::string_view unparsed{};
    };

    /// <summary>
    /// Parse status result
    /// </summary>
    enum class ParseStatus : uint8_t { Ok = 0, Malformed };

    /// <summary>
    /// Complete parsed combat log line
    /// </summary>
    struct CombatLine {
        /// <summary>
        /// TimeStamp of the line event
        /// </summary>
        TimeStamp t{};
        /// <summary>
		/// Entity that caused the event
        /// </summary>
        Entity source{};
        /// <summary>
		/// Entity that is the target of the event
        /// </summary>
        Entity target{};
        /// <summary>
		/// Ability or effect associated with the event
        /// </summary>
        NamedId   ability{};
        /// <summary>
		/// Event Type that occurred
        /// </summary>
        EventEffect event{};
        /// <summary>
		/// Detailed trailing data (value, mitigation, threat, charges) and values
        /// </summary>
        Trailing tail{};

        /// <summary>
        /// AreaEntered event data (populated when applicable)
        /// </summary>
        AreaEnteredData area_entered{};
        /// <summary>
        /// DisciplineChanged event data (populated when applicable)
        /// </summary>
        DisciplineChangedData discipline_changed{};

        /// <summary>
        /// Print combat line as formatted string
        /// </summary>
        /// <param name="line_padding">Padding to add before each line</param>
        /// <returns>Formatted string representation</returns>
        inline std::string print(std::string line_padding = "") const {
            std::ostringstream oss;
            oss << line_padding << "----- Combat Log Line -----\n";
            oss << line_padding << "Time:         " << t.to_time_point() << "\n";
            oss << line_padding << "Source:       " << source.display << "\n";
            oss << line_padding << "Target:       " << target.display << "\n";
            oss << line_padding << "Ability:      " << ability.name << "\n";
            oss << line_padding << "Event Type:   " << event.type_name << "\n";
            oss << line_padding << "Event Action: " << event.action_name << "\n";
            oss << line_padding << "Tail:         " << tail.unparsed << "\n";
            oss << line_padding << "---------------------------\n";
            return oss.str();
        }

    };


    /// <summary>
    /// Compare combat line with event type
    /// </summary>
    /// <param name="p">Combat line</param>
    /// <param name="et">Event type</param>
    /// <returns>True if event types match</returns>
    inline bool operator==(const CombatLine& p, const EventType& et) { return p.event.matches(et); }
    /// <summary>
    /// Compare combat line with event action type
    /// </summary>
    /// <param name="p">Combat line</param>
    /// <param name="eat">Event action type</param>
    /// <returns>True if event action types match</returns>
    inline bool operator==(const CombatLine& p, const EventActionType& eat) { return p.event.matches(eat); }

    /// <summary>
    /// Parse a single SWTOR combat log line into CombatLine structure
    /// </summary>
    /// <param name="line">Raw combat log line string</param>
    /// <param name="out">Output CombatLine structure</param>
    /// <returns>Parse status</returns>
    ParseStatus parse_combat_line(std::string_view line, CombatLine& out);

	/// <summary>
	/// Deduce combat role from discipline
	/// </summary>
	/// <param name="disc">Discipline enum value</param>
	/// <returns>Combat role</returns>
	extern CombatRole deduce_combat_role(Discipline disc);

    /// <summary>
    /// Options for text formatting
    /// </summary>
    struct PrintOptions {
        /// <summary>
        /// Use multiline format
        /// </summary>
        bool multiline = true;
        /// <summary>
        /// Include position data
        /// </summary>
        bool include_positions = false;
        /// <summary>
        /// Include health data
        /// </summary>
        bool include_health = false;
    };

    /// <summary>
    /// Convert combat line to human-readable text
    /// </summary>
    /// <param name="L">Combat line to format</param>
    /// <param name="opt">Formatting options</param>
    /// <returns>Formatted text string</returns>
    std::string to_text(const CombatLine& L, const PrintOptions& opt = {});

    namespace detail_json {
        /// <summary>
        /// Simple arena that owns backing storage for string_views
        /// </summary>
        struct StringArena {
            /// <summary>
            /// Internal string buffer
            /// </summary>
            std::string buf;
            /// <summary>
            /// Intern a string view into the arena
            /// </summary>
            /// <param name="s">String view to intern</param>
            /// <returns>Interned string view</returns>
            std::string_view intern(std::string_view s) {
                const size_t off = buf.size();
                buf.append(s.data(), s.size());
                return std::string_view(buf.data() + off, s.size());
            }
        };
    }

    /// <summary>
    /// Serialize a CombatLine to compact JSON
    /// </summary>
    /// <param name="L">Combat line to serialize</param>
    /// <returns>JSON string</returns>
    std::string to_json(const CombatLine& L);

    /// <summary>
    /// Parse JSON back into CombatLine
    /// </summary>
    /// <param name="json">JSON string to parse</param>
    /// <param name="out">Output CombatLine structure</param>
    /// <param name="arena">String arena for storage</param>
    /// <returns>True if parsing succeeded</returns>
    bool from_json(std::string_view json, CombatLine& out, detail_json::StringArena& arena);

    /// <summary>
    /// Deep copy CombatLine with string views interned into arena
    /// </summary>
    /// <param name="dst">Destination combat line</param>
    /// <param name="src">Source combat line</param>
    /// <param name="arena">String arena for storage</param>
    void deep_bind_into(CombatLine& dst, const CombatLine& src, detail_json::StringArena& arena);

    
    /// <summary>
    /// Convert mitigation flags to string representation
    /// </summary>
    /// <param name="f">Mitigation flags</param>
    /// <returns>String representation</returns>
    std::string flags_to_string(MitigationFlags f);

    /// <summary>
    /// Parse a NamedId from "Name {ID}" format
    /// </summary>
    /// <param name="text">Text to parse</param>
    /// <param name="out">Output NamedId</param>
    /// <returns>True if parsing succeeded</returns>
    inline bool parse_named_id(std::string_view text, NamedId& out);

    namespace detail {

        /// <summary>
        /// Parse a NamedId from "Name {ID}" format
        /// </summary>
        /// <param name="text">Text to parse</param>
        /// <param name="out">Output NamedId</param>
        /// <returns>True if parsing succeeded</returns>
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

        /// <summary>
        /// Parse AreaEntered event data
        /// </summary>
        /// <param name="event_text">Event text from log</param>
        /// <param name="value_text">Value text from log</param>
        /// <param name="out">Output AreaEnteredData</param>
        /// <returns>True if parsing succeeded</returns>
        inline bool parse_area_entered(std::string_view event_text,
            std::string_view value_text,
            AreaEnteredData& out);

        /// <summary>
        /// Parse version tag from text
        /// </summary>
        /// <param name="text">Text containing version tag</param>
        /// <param name="out">Output version string</param>
        /// <returns>True if parsing succeeded</returns>
        inline bool parse_version_tag(std::string_view text, std::string& out) {
            if (text.empty() || text.front() != '<' || text.back() != '>') return false;
            out = std::string(text.substr(1, text.size() - 2));
            return true;
        }

        /// <summary>
        /// Parse DisciplineChanged event data
        /// </summary>
        /// <param name="event_text">Event text from log</param>
        /// <param name="out">Output DisciplineChangedData</param>
        /// <returns>True if parsing succeeded</returns>
        inline bool parse_discipline_changed(std::string_view event_text,
            DisciplineChangedData& out);

    }


    /// <summary>
    /// Parse mitigation information from trailing text
    /// </summary>
    /// <param name="rest">Trailing text to parse</param>
    /// <param name="vf">Output value field</param>
    static inline void parse_mitigation_tail(std::string_view rest, ValueField& vf);
    
    /// <summary>
    /// Check if a CombatLine is an AreaEntered event
    /// </summary>
    /// <param name="line">Combat line to check</param>
    /// <returns>True if AreaEntered event</returns>
    inline bool is_area_entered(const CombatLine& line) {
		return line.event == swtor::EventType::AreaEntered;
    }

    /// <summary>
    /// Check if a CombatLine is a DisciplineChanged event
    /// </summary>
    /// <param name="line">Combat line to check</param>
    /// <returns>True if DisciplineChanged event</returns>
    inline bool is_discipline_changed(const CombatLine& line) {
		return line.event == swtor::EventType::DisciplineChanged;
    }

    /// <summary>
    /// Get area name from AreaEntered event
    /// </summary>
    /// <param name="line">Combat line</param>
    /// <returns>Area name or empty string if not AreaEntered</returns>
    inline std::string_view get_area_name(const CombatLine& line) {
        if (line.event == EventType::AreaEntered) {
            return line.area_entered.area.name;
        }
        return "";
    }

    /// <summary>
    /// Get difficulty name from AreaEntered event
    /// </summary>
    /// <param name="line">Combat line</param>
    /// <returns>Difficulty name or empty string if not present</returns>
    inline std::string_view get_difficulty_name(const CombatLine& line) {
        if (line.event == EventType::AreaEntered && line.area_entered.has_difficulty) {
            return line.area_entered.difficulty.name;
        }
        return "";
    }

    /// <summary>
    /// Get the combat class name as a string
    /// </summary>
    /// <param name="cls">Combat class enum value</param>
    /// <returns>Combat class name</returns>
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

    /// <summary>
    /// Get the discipline name as a string
    /// </summary>
    /// <param name="disc">Discipline enum value</param>
    /// <returns>Discipline name</returns>
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


    /// <summary>
    /// Format a duration in milliseconds as a human-readable string
    /// </summary>
    /// <param name="totalMs">Duration in milliseconds</param>
    /// <returns>Formatted duration string</returns>
    std::string formatDurationMs(std::int64_t totalMs);




} // namespace swtor
