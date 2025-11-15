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

    // C#-style interface (pure-virtual) for accumulating and querying player stats.
    struct stat_keeper {
        // ---------- Lifecycle ----------
        virtual ~stat_keeper() = default;

        // Choose which player we’re tracking (by id or name).
        virtual void set_tracked_entity(const EntityId& id) = 0;

        // Feed parsed lines. Implementation may ignore lines unrelated to the tracked player.
        virtual void ingest(const CombatLine& line) = 0;

        // Clear all state.
        virtual void reset() = 0;

        // ---------- Summary “views” (lightweight PODs you can dump to JSON/UI) ----------
        struct Duration {
            uint32_t ms{ 0 };
            double   seconds() const { return ms / 1000.0; }
        };

        struct Rotation {
            uint32_t actions{ 0 };      // total player-initiated actions (abilities)
            double   apm{ 0.0 };        // actions per minute
            uint32_t gcd_count{ 0 };
            uint32_t cast_time_ms{ 0 }; // total time spent casting/channelling
            uint32_t idle_time_ms{ 0 }; // time with no actions
        };

        struct DamageStats {
            int64_t total{ 0 };
            double  dps{ 0.0 };
            int64_t largest_hit{ 0 };
            double  crit_rate{ 0.0 };     // fraction [0..1]
            double  hit_rate{ 0.0 };      // non-miss/immune/resist fraction
            double  shielded_pct{ 0.0 };  // fraction of damage flagged as shielded
        };

        struct HealingStats {
            int64_t total{ 0 };
            int64_t effective{ 0 };       // after overheal
            int64_t overheal{ 0 };
            double  hps{ 0.0 };
            double  overheal_pct{ 0.0 };  // overheal / total
            int64_t largest_heal{ 0 };
            double  crit_rate{ 0.0 };
            int64_t absorb_contrib{ 0 };  // shielding/absorbs granted
        };

        struct TakenStats {
            int64_t total_taken{ 0 };
            double  dtps{ 0.0 };
            uint32_t deaths{ 0 };
            uint32_t time_dead_ms{ 0 };

            // mitigation outcome counts
            uint32_t defended{ 0 };   // dodge/parry/deflect totalled
            uint32_t shielded{ 0 };
            uint32_t resisted{ 0 };
            uint32_t missed{ 0 };
            uint32_t immune{ 0 };
        };

        struct ThreatStats {
            bool    available{ false };   // true if logs carried threat deltas
            int64_t total_threat{ 0 };
            double  tps{ 0.0 };
            uint32_t taunts{ 0 };
            uint32_t successful_taunts{ 0 };
            double  taunt_accuracy{ 0.0 }; // [0..1]
        };

        struct ResourceStats {
            bool    available{ false };
            double  avg_level{ 0.0 };           // average resource level (class-normalized if impl chooses)
            uint32_t time_below_threshold_ms{ 0 };
            uint32_t capped_waste{ 0 };         // amount wasted at resource cap
            uint32_t restores_used{ 0 };        // adrenals/relics/on-use etc.
        };

        struct PositionStats {
            bool    available{ false };
            double  distance_travelled_m{ 0.0 };
            double  time_in_melee_range_pct{ 0.0 };
            double  time_behind_target_pct{ 0.0 };
        };

        struct MechanicsStats {
            uint32_t interrupts{ 0 };
            uint32_t cleanses{ 0 };
            uint32_t combat_res{ 0 };
            uint32_t guards_swapped{ 0 };
        };

        struct UptimeRow {
            NamedId effect{};
            double  uptime_pct{ 0.0 }; // [0..1] over encounter duration
            int     max_stacks{ 0 };
        };

        struct AbilityRow {
            NamedId ability{};
            int64_t total{ 0 };
            uint32_t hits{ 0 };
            double  avg{ 0.0 };
            double  crit_rate{ 0.0 };
            double  share_pct{ 0.0 }; // fraction of total
        };

        struct SourceRow {  // for “damage taken from” or “healing received from”
            NamedId source{};
            int64_t total{ 0 };
            double  share_pct{ 0.0 };
        };

        struct Summary {
            std::string encounter_name{};
            Duration    duration{};
            Rotation    rotation{};
            DamageStats damage{};
            HealingStats healing{};
            TakenStats  taken{};
            ThreatStats threat{};
            ResourceStats resource{};
            PositionStats position{};
            MechanicsStats mechanics{};
            uint32_t     unique_targets_hit{ 0 };
        };

        // ---------- Summary getters ----------
        virtual Summary       get_summary() const = 0;
        virtual Duration      get_duration() const = 0;
        virtual Rotation      get_rotation() const = 0;
        virtual DamageStats   get_damage() const = 0;
        virtual HealingStats  get_healing() const = 0;
        virtual TakenStats    get_taken() const = 0;
        virtual ThreatStats   get_threat() const = 0;
        virtual ResourceStats get_resource() const = 0;
        virtual PositionStats get_position() const = 0;
        virtual MechanicsStats get_mechanics() const = 0;

        // ---------- Breakdowns / tables ----------
        // Top N damaging abilities used by the tracked player.
        virtual std::vector<AbilityRow> top_damage_abilities(size_t top_n = 10) const = 0;

        // Top N sources of damage taken (by enemy/ability).
        virtual std::vector<AbilityRow> top_damage_taken_by_ability(size_t top_n = 10) const = 0;
        virtual std::vector<SourceRow>  top_damage_taken_by_source(size_t top_n = 10) const = 0;

        // Top N healing abilities (done) and healing received (by source/ability).
        virtual std::vector<AbilityRow> top_healing_abilities(size_t top_n = 10) const = 0;
        virtual std::vector<SourceRow>  top_healing_received_by_source(size_t top_n = 10) const = 0;

        // Buffs/Debuffs/HoTs/DoTs uptime (apply/remove effects).
        virtual std::vector<UptimeRow>  buff_uptimes() const = 0;
        virtual std::vector<UptimeRow>  debuff_uptimes_on_primary_target() const = 0;

        // Burst windows (e.g., max rolling 10s/30s DPS/HPS). Implementations may compute lazily.
        struct BurstWindow { uint32_t window_ms{ 0 }; double peak_rate{ 0.0 }; };
        virtual std::optional<BurstWindow> peak_dps_window(uint32_t window_ms) const = 0;
        virtual std::optional<BurstWindow> peak_hps_window(uint32_t window_ms) const = 0;

        // ---------- Raw counters helpful for UI tooling ----------
        virtual uint32_t interrupts() const = 0;
        virtual uint32_t cleanses() const = 0;
        virtual uint32_t taunts() const = 0;
        virtual uint32_t successful_taunts() const = 0;
        virtual uint32_t deaths() const = 0;
        virtual uint32_t time_dead_ms() const = 0;

        // ---------- Export helpers ----------
        // Compact JSON blobs for quick UI consumption (schema is implementation-defined).
        virtual std::string to_json_summary() const = 0;
        virtual std::string to_json_tables()  const = 0;
    };

} // namespace swtor
