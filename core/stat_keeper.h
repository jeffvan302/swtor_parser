#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <cstdlib>


#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include "swtor_parser.h"

namespace swtor {

    /// <summary>
    /// Interface for accumulating and querying player statistics from combat logs
    /// </summary>
    struct stat_keeper {
        /// <summary>
        /// Virtual destructor
        /// </summary>
        virtual ~stat_keeper() = default;

        /// <summary>
        /// Choose which player to track by entity ID
        /// </summary>
        /// <param name="id">Entity ID of the player to track</param>
        virtual void set_tracked_entity(const uint64_t id) = 0;

        /// <summary>
        /// Feed parsed combat lines to accumulate statistics
        /// </summary>
        /// <param name="line">Combat line to process</param>
        virtual void ingest(const CombatLine& line) = 0;

        /// <summary>
        /// Clear all accumulated state
        /// </summary>
        virtual void reset() = 0;

        /// <summary>
        /// Represents a duration in milliseconds
        /// </summary>
        struct Duration {
            /// <summary>
            /// Duration in milliseconds
            /// </summary>
            uint32_t ms{ 0 };
            /// <summary>
            /// Convert duration to seconds
            /// </summary>
            /// <returns>Duration in seconds</returns>
            double   seconds() const { return ms / 1000.0; }
        };

        /// <summary>
        /// Statistics about ability rotation and action economy
        /// </summary>
        struct Rotation {
            /// <summary>
            /// Total player-initiated actions (abilities)
            /// </summary>
            uint32_t actions{ 0 };
            /// <summary>
            /// Actions per minute
            /// </summary>
            double   apm{ 0.0 };
            /// <summary>
            /// Number of GCD-triggering actions
            /// </summary>
            uint32_t gcd_count{ 0 };
            /// <summary>
            /// Total time spent casting/channeling in milliseconds
            /// </summary>
            uint32_t cast_time_ms{ 0 };
            /// <summary>
            /// Time with no actions in milliseconds
            /// </summary>
            uint32_t idle_time_ms{ 0 };
        };

        /// <summary>
        /// Damage output statistics
        /// </summary>
        struct DamageStats {
            /// <summary>
            /// Total damage dealt
            /// </summary>
            int64_t total{ 0 };
            /// <summary>
            /// Damage per second
            /// </summary>
            double  dps{ 0.0 };
            /// <summary>
            /// Largest single hit
            /// </summary>
            int64_t largest_hit{ 0 };
            /// <summary>
            /// Critical hit rate (0.0 to 1.0)
            /// </summary>
            double  crit_rate{ 0.0 };
            /// <summary>
            /// Non-miss/immune/resist fraction (0.0 to 1.0)
            /// </summary>
            double  hit_rate{ 0.0 };
            /// <summary>
            /// Fraction of damage flagged as shielded (0.0 to 1.0)
            /// </summary>
            double  shielded_pct{ 0.0 };
        };

        /// <summary>
        /// Healing output statistics
        /// </summary>
        struct HealingStats {
            /// <summary>
            /// Total healing done
            /// </summary>
            int64_t total{ 0 };
            /// <summary>
            /// Effective healing after overheal
            /// </summary>
            int64_t effective{ 0 };
            /// <summary>
            /// Amount of overheal
            /// </summary>
            int64_t overheal{ 0 };
            /// <summary>
            /// Healing per second
            /// </summary>
            double  hps{ 0.0 };
            /// <summary>
            /// Overheal percentage (overheal / total)
            /// </summary>
            double  overheal_pct{ 0.0 };
            /// <summary>
            /// Largest single heal
            /// </summary>
            int64_t largest_heal{ 0 };
            /// <summary>
            /// Critical heal rate (0.0 to 1.0)
            /// </summary>
            double  crit_rate{ 0.0 };
            /// <summary>
            /// Shielding/absorbs granted to others
            /// </summary>
            int64_t absorb_contrib{ 0 };
        };

        /// <summary>
        /// Damage and effects taken statistics
        /// </summary>
        struct TakenStats {
            /// <summary>
            /// Total damage taken
            /// </summary>
            int64_t total_taken{ 0 };
            /// <summary>
            /// Damage taken per second
            /// </summary>
            double  dtps{ 0.0 };
            /// <summary>
            /// Number of deaths
            /// </summary>
            uint32_t deaths{ 0 };
            /// <summary>
            /// Time spent dead in milliseconds
            /// </summary>
            uint32_t time_dead_ms{ 0 };

            /// <summary>
            /// Total defended attacks (dodge/parry/deflect)
            /// </summary>
            uint32_t defended{ 0 };
            /// <summary>
            /// Number of attacks shielded
            /// </summary>
            uint32_t shielded{ 0 };
            /// <summary>
            /// Number of attacks resisted
            /// </summary>
            uint32_t resisted{ 0 };
            /// <summary>
            /// Number of attacks missed
            /// </summary>
            uint32_t missed{ 0 };
            /// <summary>
            /// Number of attacks immuned
            /// </summary>
            uint32_t immune{ 0 };
        };

        /// <summary>
        /// Threat generation statistics
        /// </summary>
        struct ThreatStats {
            /// <summary>
            /// True if threat data is available in logs
            /// </summary>
            bool    available{ false };
            /// <summary>
            /// Total threat generated
            /// </summary>
            int64_t total_threat{ 0 };
            /// <summary>
            /// Threat per second
            /// </summary>
            double  tps{ 0.0 };
            /// <summary>
            /// Number of taunt attempts
            /// </summary>
            uint32_t taunts{ 0 };
            /// <summary>
            /// Number of successful taunts
            /// </summary>
            uint32_t successful_taunts{ 0 };
            /// <summary>
            /// Taunt accuracy rate (0.0 to 1.0)
            /// </summary>
            double  taunt_accuracy{ 0.0 };
        };

        /// <summary>
        /// Resource management statistics (energy, force, heat, etc.)
        /// </summary>
        struct ResourceStats {
            /// <summary>
            /// True if resource data is available
            /// </summary>
            bool    available{ false };
            /// <summary>
            /// Average resource level
            /// </summary>
            double  avg_level{ 0.0 };
            /// <summary>
            /// Time spent below resource threshold in milliseconds
            /// </summary>
            uint32_t time_below_threshold_ms{ 0 };
            /// <summary>
            /// Amount of resource wasted at cap
            /// </summary>
            uint32_t capped_waste{ 0 };
            /// <summary>
            /// Number of resource restores used (adrenals/relics/on-use)
            /// </summary>
            uint32_t restores_used{ 0 };
        };

        /// <summary>
        /// Position and movement statistics
        /// </summary>
        struct PositionStats {
            /// <summary>
            /// True if position data is available
            /// </summary>
            bool    available{ false };
            /// <summary>
            /// Total distance travelled in meters
            /// </summary>
            double  distance_travelled_m{ 0.0 };
            /// <summary>
            /// Percentage of time in melee range (0.0 to 1.0)
            /// </summary>
            double  time_in_melee_range_pct{ 0.0 };
            /// <summary>
            /// Percentage of time behind target (0.0 to 1.0)
            /// </summary>
            double  time_behind_target_pct{ 0.0 };
        };

        /// <summary>
        /// Combat mechanics statistics
        /// </summary>
        struct MechanicsStats {
            /// <summary>
            /// Number of interrupts performed
            /// </summary>
            uint32_t interrupts{ 0 };
            /// <summary>
            /// Number of cleanses performed
            /// </summary>
            uint32_t cleanses{ 0 };
            /// <summary>
            /// Number of combat resurrections performed
            /// </summary>
            uint32_t combat_res{ 0 };
            /// <summary>
            /// Number of guard swaps performed
            /// </summary>
            uint32_t guards_swapped{ 0 };
        };

        /// <summary>
        /// Uptime information for a buff/debuff effect
        /// </summary>
        struct UptimeRow {
            /// <summary>
            /// Effect identifier
            /// </summary>
            NamedId effect{};
            /// <summary>
            /// Uptime percentage over encounter duration (0.0 to 1.0)
            /// </summary>
            double  uptime_pct{ 0.0 };
            /// <summary>
            /// Maximum number of stacks observed
            /// </summary>
            int     max_stacks{ 0 };
        };

        /// <summary>
        /// Statistics for a specific ability
        /// </summary>
        struct AbilityRow {
            /// <summary>
            /// Ability identifier
            /// </summary>
            NamedId ability{};
            /// <summary>
            /// Total damage or healing from this ability
            /// </summary>
            int64_t total{ 0 };
            /// <summary>
            /// Number of hits
            /// </summary>
            uint32_t hits{ 0 };
            /// <summary>
            /// Average damage or healing per hit
            /// </summary>
            double  avg{ 0.0 };
            /// <summary>
            /// Critical hit rate (0.0 to 1.0)
            /// </summary>
            double  crit_rate{ 0.0 };
            /// <summary>
            /// Fraction of total damage or healing (0.0 to 1.0)
            /// </summary>
            double  share_pct{ 0.0 };
        };

        /// <summary>
        /// Statistics for damage taken from or healing received from a source
        /// </summary>
        struct SourceRow {
            /// <summary>
            /// Source identifier
            /// </summary>
            NamedId source{};
            /// <summary>
            /// Total damage or healing from this source
            /// </summary>
            int64_t total{ 0 };
            /// <summary>
            /// Fraction of total damage or healing (0.0 to 1.0)
            /// </summary>
            double  share_pct{ 0.0 };
        };

        /// <summary>
        /// Complete summary of all statistics
        /// </summary>
        struct Summary {
            /// <summary>
            /// Name of the encounter
            /// </summary>
            std::string encounter_name{};
            /// <summary>
            /// Encounter duration
            /// </summary>
            Duration    duration{};
            /// <summary>
            /// Rotation statistics
            /// </summary>
            Rotation    rotation{};
            /// <summary>
            /// Damage statistics
            /// </summary>
            DamageStats damage{};
            /// <summary>
            /// Healing statistics
            /// </summary>
            HealingStats healing{};
            /// <summary>
            /// Damage taken statistics
            /// </summary>
            TakenStats  taken{};
            /// <summary>
            /// Threat statistics
            /// </summary>
            ThreatStats threat{};
            /// <summary>
            /// Resource statistics
            /// </summary>
            ResourceStats resource{};
            /// <summary>
            /// Position statistics
            /// </summary>
            PositionStats position{};
            /// <summary>
            /// Mechanics statistics
            /// </summary>
            MechanicsStats mechanics{};
            /// <summary>
            /// Number of unique targets hit
            /// </summary>
            uint32_t     unique_targets_hit{ 0 };
        };

        /// <summary>
        /// Get complete summary of all statistics
        /// </summary>
        /// <returns>Summary structure containing all stats</returns>
        virtual Summary       get_summary() const = 0;
        /// <summary>
        /// Get encounter duration
        /// </summary>
        /// <returns>Duration structure</returns>
        virtual Duration      get_duration() const = 0;
        /// <summary>
        /// Get rotation statistics
        /// </summary>
        /// <returns>Rotation statistics structure</returns>
        virtual Rotation      get_rotation() const = 0;
        /// <summary>
        /// Get damage statistics
        /// </summary>
        /// <returns>Damage statistics structure</returns>
        virtual DamageStats   get_damage() const = 0;
        /// <summary>
        /// Get healing statistics
        /// </summary>
        /// <returns>Healing statistics structure</returns>
        virtual HealingStats  get_healing() const = 0;
        /// <summary>
        /// Get damage taken statistics
        /// </summary>
        /// <returns>Damage taken statistics structure</returns>
        virtual TakenStats    get_taken() const = 0;
        /// <summary>
        /// Get threat statistics
        /// </summary>
        /// <returns>Threat statistics structure</returns>
        virtual ThreatStats   get_threat() const = 0;
        /// <summary>
        /// Get resource statistics
        /// </summary>
        /// <returns>Resource statistics structure</returns>
        virtual ResourceStats get_resource() const = 0;
        /// <summary>
        /// Get position statistics
        /// </summary>
        /// <returns>Position statistics structure</returns>
        virtual PositionStats get_position() const = 0;
        /// <summary>
        /// Get mechanics statistics
        /// </summary>
        /// <returns>Mechanics statistics structure</returns>
        virtual MechanicsStats get_mechanics() const = 0;

        /// <summary>
        /// Get top N damaging abilities used by the tracked player
        /// </summary>
        /// <param name="top_n">Number of top abilities to return (default 10)</param>
        /// <returns>Vector of ability statistics sorted by total damage</returns>
        virtual std::vector<AbilityRow> top_damage_abilities(size_t top_n = 10) const = 0;

        /// <summary>
        /// Get top N damage sources taken by ability
        /// </summary>
        /// <param name="top_n">Number of top abilities to return (default 10)</param>
        /// <returns>Vector of ability statistics sorted by damage taken</returns>
        virtual std::vector<AbilityRow> top_damage_taken_by_ability(size_t top_n = 10) const = 0;
        /// <summary>
        /// Get top N damage sources taken by source entity
        /// </summary>
        /// <param name="top_n">Number of top sources to return (default 10)</param>
        /// <returns>Vector of source statistics sorted by damage taken</returns>
        virtual std::vector<SourceRow>  top_damage_taken_by_source(size_t top_n = 10) const = 0;

        /// <summary>
        /// Get top N healing abilities used by the tracked player
        /// </summary>
        /// <param name="top_n">Number of top abilities to return (default 10)</param>
        /// <returns>Vector of ability statistics sorted by total healing</returns>
        virtual std::vector<AbilityRow> top_healing_abilities(size_t top_n = 10) const = 0;
        /// <summary>
        /// Get top N healing sources received by source entity
        /// </summary>
        /// <param name="top_n">Number of top sources to return (default 10)</param>
        /// <returns>Vector of source statistics sorted by healing received</returns>
        virtual std::vector<SourceRow>  top_healing_received_by_source(size_t top_n = 10) const = 0;

        /// <summary>
        /// Get buff uptimes for the tracked player
        /// </summary>
        /// <returns>Vector of uptime statistics for buffs</returns>
        virtual std::vector<UptimeRow>  buff_uptimes() const = 0;
        /// <summary>
        /// Get debuff uptimes on primary target
        /// </summary>
        /// <returns>Vector of uptime statistics for debuffs</returns>
        virtual std::vector<UptimeRow>  debuff_uptimes_on_primary_target() const = 0;

        /// <summary>
        /// Represents a burst damage or healing window
        /// </summary>
        struct BurstWindow {
            /// <summary>
            /// Window size in milliseconds
            /// </summary>
            uint32_t window_ms{ 0 };
            /// <summary>
            /// Peak rate (DPS or HPS) during the window
            /// </summary>
            double peak_rate{ 0.0 };
        };
        /// <summary>
        /// Get peak DPS window of specified duration
        /// </summary>
        /// <param name="window_ms">Window size in milliseconds</param>
        /// <returns>Optional burst window with peak DPS if available</returns>
        virtual std::optional<BurstWindow> peak_dps_window(uint32_t window_ms) const = 0;
        /// <summary>
        /// Get peak HPS window of specified duration
        /// </summary>
        /// <param name="window_ms">Window size in milliseconds</param>
        /// <returns>Optional burst window with peak HPS if available</returns>
        virtual std::optional<BurstWindow> peak_hps_window(uint32_t window_ms) const = 0;

        /// <summary>
        /// Get total number of interrupts performed
        /// </summary>
        /// <returns>Interrupt count</returns>
        virtual uint32_t interrupts() const = 0;
        /// <summary>
        /// Get total number of cleanses performed
        /// </summary>
        /// <returns>Cleanse count</returns>
        virtual uint32_t cleanses() const = 0;
        /// <summary>
        /// Get total number of taunt attempts
        /// </summary>
        /// <returns>Taunt count</returns>
        virtual uint32_t taunts() const = 0;
        /// <summary>
        /// Get number of successful taunts
        /// </summary>
        /// <returns>Successful taunt count</returns>
        virtual uint32_t successful_taunts() const = 0;
        /// <summary>
        /// Get number of deaths
        /// </summary>
        /// <returns>Death count</returns>
        virtual uint32_t deaths() const = 0;
        /// <summary>
        /// Get time spent dead in milliseconds
        /// </summary>
        /// <returns>Time dead in milliseconds</returns>
        virtual uint32_t time_dead_ms() const = 0;

        /// <summary>
        /// Export summary statistics as compact JSON
        /// </summary>
        /// <returns>JSON string containing summary data</returns>
        virtual std::string to_json_summary() const = 0;
        /// <summary>
        /// Export detailed tables as compact JSON
        /// </summary>
        /// <returns>JSON string containing table data</returns>
        virtual std::string to_json_tables()  const = 0;
    };

} // namespace swtor
