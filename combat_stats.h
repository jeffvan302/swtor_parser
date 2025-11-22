#pragma once
#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <optional>
#include <deque>
#include <algorithm>
#include "swtor_parser.h"
#include "stat_keeper.h"

namespace swtor {
    // Hash + equality for NamedId (prefer id when present, otherwise name)
    struct NamedIdHasher {
        size_t operator()(const NamedId& k) const noexcept {
            if (k.id != 0) return std::hash<uint64_t>{}(k.id);
            return std::hash<std::string_view>{}(k.name);
        }
    };

    struct NamedIdEqual {
        bool operator()(const NamedId& a, const NamedId& b) const noexcept {
            if (a.id != 0 || b.id != 0) return a.id == b.id; // if either has an id, compare ids
            return a.name == b.name;                          // otherwise fall back to name
        }
    };


    class combat_stats final : public stat_keeper {
    public:
        // ---------- lifecycle ----------
        ~combat_stats() override = default;

        void set_tracked_entity(const uint64_t id) override;

        void ingest(const CombatLine& line) override;
        void reset() override;

        // ---------- summaries ----------
        Summary       get_summary() const override;
        Duration      get_duration() const override;
        Rotation      get_rotation() const override;
        DamageStats   get_damage() const override;
        HealingStats  get_healing() const override;
        TakenStats    get_taken() const override;
        ThreatStats   get_threat() const override;
        ResourceStats get_resource() const override;
        PositionStats get_position() const override;
        MechanicsStats get_mechanics() const override;

        // ---------- breakdowns ----------
        std::vector<AbilityRow> top_damage_abilities(size_t top_n) const override;
        std::vector<AbilityRow> top_damage_taken_by_ability(size_t top_n) const override;
        std::vector<SourceRow>  top_damage_taken_by_source(size_t top_n) const override;
        std::vector<AbilityRow> top_healing_abilities(size_t top_n) const override;
        std::vector<SourceRow>  top_healing_received_by_source(size_t top_n) const override;

        std::vector<UptimeRow>  buff_uptimes() const override;
        std::vector<UptimeRow>  debuff_uptimes_on_primary_target() const override;

        std::optional<BurstWindow> peak_dps_window(uint32_t window_ms) const override;
        std::optional<BurstWindow> peak_hps_window(uint32_t window_ms) const override;

        // ---------- raw counters ----------
        uint32_t interrupts() const override { return mech_.interrupts; }
        uint32_t cleanses()   const override { return mech_.cleanses; }
        uint32_t taunts()     const override { return threat_.taunts; }
        uint32_t successful_taunts() const override { return threat_.successful_taunts; }
        uint32_t deaths()     const override { return taken_.deaths; }
        uint32_t time_dead_ms() const override { return taken_.time_dead_ms; }

        // ---------- export ----------
        std::string to_json_summary() const override;
        std::string to_json_tables()  const override;

    private:
        // --- helpers & state ---
        struct Tally {
            int64_t total{ 0 };
            uint32_t hits{ 0 };
            uint32_t crits{ 0 };
            int64_t largest{ 0 };
            uint32_t shielded_flags{ 0 };
        };

        struct EventPoint { uint32_t t; int64_t v; };  // for burst windows

        struct EffectState {
            // Tracks active windows for uptime calculations
            uint32_t active_since_ms{ 0 };
            uint32_t total_active_ms{ 0 };
            int max_stacks{ 0 };
            int curr_stacks{ 0 };
            bool active{ false };
        };

        // identity & encounter
        uint64_t tracked_id_;
        std::string tracked_name_;
        std::string encounter_name_;
        uint32_t start_ms_{ 0 };
        uint32_t end_ms_{ 0 };
        bool encounter_open_{ false };

        // rotation
        Rotation rot_{};
        uint32_t last_action_ms_{ 0 };

        // damage/healing/taken
        DamageStats dmg_{};
        HealingStats heal_{};
        TakenStats taken_{};

        // maps for tables
        std::unordered_map<NamedId, Tally, NamedIdHasher> dmg_by_ability_;
        std::unordered_map<NamedId, Tally, NamedIdHasher> heal_by_ability_;
        std::unordered_map<NamedId, Tally, NamedIdHasher> taken_by_ability_;
        std::unordered_map<NamedId, int64_t, NamedIdHasher> taken_by_source_;
        std::unordered_map<NamedId, int64_t, NamedIdHasher> healing_received_by_source_;

        // threat/resource/position/mechanics (light v1)
        ThreatStats threat_{};
        ResourceStats resource_{};
        PositionStats position_{}; // not computed in v1
        MechanicsStats mech_{};

        // uptime tracking (effects on the tracked entity, and effects applied by the entity)
        std::unordered_map<NamedId, EffectState, NamedIdHasher> self_buffs_;
        std::unordered_map<NamedId, EffectState, NamedIdHasher> target_debuffs_;

        // event series for burst windows
        std::deque<EventPoint> dmg_series_;   // damage done points
        std::deque<EventPoint> heal_series_;  // healing done points

    private:
        bool is_tracked_source(const CombatLine& l) const;
        bool is_tracked_target(const CombatLine& l) const;
        void on_damage_done(const CombatLine& l, const ValueField& v);
        void on_heal_done(const CombatLine& l, const ValueField& v);
        void on_damage_taken(const CombatLine& l, const ValueField& v);
        void on_heal_received(const CombatLine& l, const ValueField& v);
        void on_effect_apply_remove(const CombatLine& l);
        void on_mechanic(const CombatLine& l);
        void on_threat(const CombatLine& l);

        static double safe_div(double num, double den) { return den > 0.0 ? (num / den) : 0.0; }
        template<typename Map, typename RowMaker>
        static std::vector<typename std::invoke_result_t<RowMaker, typename Map::value_type>>
            top_n_from_map(const Map& m, size_t n, RowMaker row);
    };

} // namespace swtor
