#include "combat_stats.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <queue>

namespace swtor {

    // ---------- identity ----------

    void combat_stats::set_tracked_entity(const EntityId& id) {
        tracked_id_ = id;
    }


    // ---------- ingest ----------

    void combat_stats::ingest(const CombatLine& l) {

        if (is_tracked_source(l) ) {
            switch (l.evt.kind_id)
            {
			case swtor::Evt::Damage:

				break;
            default:
                break;
            }
		} else if (is_tracked_target(l)) {
            
            
		}
        // Resolve tracked id from name the first time we see it
        if (!tracked_id_.has_value() && !tracked_name_.empty()) {
            if (l.source.name == tracked_name_) tracked_id_ = l.source.id;
            else if (l.target.name == tracked_name_) tracked_id_ = l.target.id;
        }

        // rotation-ish: count ability events by the tracked source
        if (is_tracked_source(l) && l.effect.kind == EffectKind::Event && l.ability.id != 0) {
            rot_.actions++;
            if (l.time.combat_ms > last_action_ms_) {
                // crude idle-time estimate (gap between actions)
                const uint32_t gap = l.time.combat_ms - last_action_ms_;
                rot_.idle_time_ms += gap;
                last_action_ms_ = l.time.combat_ms;
            }
        }

        // value-bearing events
        if (l.trailing.has_value) {
            const ValueField& v = l.trailing.value;
            if (is_tracked_source(l)) {
                if (v.school.is_damage()) on_damage_done(l, v);
                if (v.school.is_heal())   on_heal_done(l, v);
            }
            if (is_tracked_target(l)) {
                if (v.school.is_damage()) on_damage_taken(l, v);
                if (v.school.is_heal())   on_heal_received(l, v);
            }
        }

        // mechanics & effects
        on_mechanic(l);
        on_effect_apply_remove(l);
        on_threat(l);

        // deaths / time-dead (track via target HP hitting 0)
        if (is_tracked_target(l) && l.target.hp.current == 0 && l.target.hp.max > 0) {
            taken_.deaths++;
            // time_dead will be accounted when we next see HP > 0 (left as TODO for precision)
        }
    }

    bool combat_stats::is_tracked_source(const CombatLine& l) const {
        if (tracked_id_) return l.source.id == *tracked_id_;
        if (!tracked_name_.empty()) return l.source.name == tracked_name_;
        return false;
    }
    bool combat_stats::is_tracked_target(const CombatLine& l) const {
        if (tracked_id_) return l.target.id == *tracked_id_;
        if (!tracked_name_.empty()) return l.target.name == tracked_name_;
        return false;
    }

    // ---------- event handlers ----------

    void combat_stats::on_damage_done(const CombatLine& l, const ValueField& v) {
        dmg_.total += v.amount;
        dmg_.largest_hit = std::max(dmg_.largest_hit, static_cast<int64_t>(v.amount));
        if (v.crit) ++dmg_.crit_rate; // temporarily count crits; normalized in get_damage()
        if (v.mitigation.shield) ++dmg_.shielded_pct;

        auto& t = dmg_by_ability_[l.ability];
        t.total += v.amount;
        t.hits++;
        t.crits += v.crit ? 1u : 0u;
        t.largest = std::max(t.largest, static_cast<int64_t>(v.amount));
        t.shielded_flags += v.mitigation.shield ? 1u : 0u;

        dmg_series_.push_back({ l.time.combat_ms, static_cast<int64_t>(v.amount) });
    }

    void combat_stats::on_heal_done(const CombatLine& l, const ValueField& v) {
        heal_.total += v.amount;
        // effective healing using target HP delta if provided
        if (l.target.hp.max > 0) {
            const int64_t missing = static_cast<int64_t>(l.target.hp.max) - static_cast<int64_t>(l.target.hp.current);
            const int64_t effective = std::min<int64_t>(missing, v.amount);
            heal_.effective += std::max<int64_t>(0, effective);
        }

        if (v.crit) ++heal_.crit_rate;

        auto& t = heal_by_ability_[l.ability];
        t.total += v.amount;
        t.hits++;
        t.crits += v.crit ? 1u : 0u;
        t.largest = std::max(t.largest, static_cast<int64_t>(v.amount));

        heal_series_.push_back({ l.time.combat_ms, static_cast<int64_t>(v.amount) });
    }

    void combat_stats::on_damage_taken(const CombatLine& l, const ValueField& v) {
        taken_.total_taken += v.amount;

        auto& t = taken_by_ability_[l.ability];
        t.total += v.amount;
        t.hits++;
        t.crits += v.crit ? 1u : 0u;
        t.largest = std::max(t.largest, static_cast<int64_t>(v.amount));

        taken_.defended += (l.trailing.value.mitigation.deflect || l.trailing.value.mitigation.parry || l.trailing.value.mitigation.dodge) ? 1u : 0u;
        taken_.shielded += l.trailing.value.mitigation.shield ? 1u : 0u;
        taken_.resisted += l.trailing.value.mitigation.resist ? 1u : 0u;
        taken_.missed += l.trailing.value.mitigation.miss ? 1u : 0u;
        taken_.immune += l.trailing.value.mitigation.immune ? 1u : 0u;

        taken_by_source_[l.source.name_id] += v.amount;
    }

    void combat_stats::on_heal_received(const CombatLine& l, const ValueField& v) {
        healing_received_by_source_[l.source.name_id] += v.amount;
    }

    void combat_stats::on_effect_apply_remove(const CombatLine& l) {
        auto upd = [&](auto& map, const NamedId& effect, bool is_apply, int stacks) {
            auto& st = map[effect];
            if (is_apply) {
                if (!st.active) { st.active = true; st.active_since_ms = l.time.combat_ms; }
                st.curr_stacks = stacks > 0 ? stacks : std::max(st.curr_stacks, 1);
                st.max_stacks = std::max(st.max_stacks, st.curr_stacks);
            }
            else {
                if (st.active) {
                    st.total_active_ms += (l.time.combat_ms - st.active_since_ms);
                    st.active = false; st.curr_stacks = 0;
                }
            }
            };

        switch (l.effect.kind) {
        case EffectKind::ApplyEffect:
            if (is_tracked_target(l)) upd(self_buffs_, l.effect.effect, true, l.trailing.has_charges ? static_cast<int>(l.trailing.charges) : 0);
            if (is_tracked_source(l)) upd(target_debuffs_, l.effect.effect, true, l.trailing.has_charges ? static_cast<int>(l.trailing.charges) : 0);
            break;
        case EffectKind::RemoveEffect:
            if (is_tracked_target(l)) upd(self_buffs_, l.effect.effect, false, 0);
            if (is_tracked_source(l)) upd(target_debuffs_, l.effect.effect, false, 0);
            break;
        default: break;
        }
    }

    void combat_stats::on_mechanic(const CombatLine& l) {
        if (!is_tracked_source(l) && !is_tracked_target(l)) return;

        if (l.effect.kind == EffectKind::Event) {
            // Cheap heuristic via ability names/ids your parser knows to mark as mechanics
            if (l.ability.is_interrupt()) ++mech_.interrupts;
            if (l.ability.is_cleanse()) ++mech_.cleanses;
            if (l.ability.is_battle_rez()) ++mech_.combat_res;
            if (l.ability.is_guard_swap()) ++mech_.guards_swapped;
        }
    }

    void combat_stats::on_threat(const CombatLine& l) {
        if (!l.trailing.has_threat) return;
        threat_.available = true;
        if (is_tracked_source(l)) {
            threat_.total_threat += l.trailing.threat;
            if (l.ability.is_taunt()) {
                ++threat_.taunts;
                // Post-taunt normalization detection is parser-specific; here we count all as successful unless resisted
                if (!l.trailing.value.mitigation.resist && !l.trailing.value.mitigation.miss) {
                    ++threat_.successful_taunts;
                }
            }
        }
    }

    // ---------- getters ----------

    stat_keeper::Duration combat_stats::get_duration() const {
        Duration d{};
        if (start_ms_ && (end_ms_ >= start_ms_)) d.ms = end_ms_ - start_ms_;
        return d;
    }

    stat_keeper::Rotation combat_stats::get_rotation() const {
        return rot_;
    }

    stat_keeper::DamageStats combat_stats::get_damage() const {
        DamageStats out = dmg_;
        // Convert crit_rate & shielded_pct which we stored as counts to actual fractions
        uint32_t total_hits = 0;
        for (const auto& [_, t] : dmg_by_ability_) total_hits += t.hits;
        out.crit_rate = safe_div(dmg_.crit_rate, static_cast<double>(total_hits));
        out.hit_rate = 1.0; // refine if you count misses separately
        out.shielded_pct = safe_div(dmg_.shielded_pct, static_cast<double>(total_hits));
        return out;
    }

    stat_keeper::HealingStats combat_stats::get_healing() const {
        HealingStats out = heal_;
        uint32_t total_hits = 0;
        for (const auto& [_, t] : heal_by_ability_) total_hits += t.hits;
        out.crit_rate = safe_div(heal_.crit_rate, static_cast<double>(total_hits));
        return out;
    }

    stat_keeper::TakenStats combat_stats::get_taken() const {
        return taken_;
    }

    stat_keeper::ThreatStats combat_stats::get_threat() const {
        ThreatStats t = threat_;
        t.taunt_accuracy = safe_div(threat_.successful_taunts, static_cast<double>(std::max(1u, threat_.taunts)));
        return t;
    }

    stat_keeper::ResourceStats combat_stats::get_resource() const {
        return resource_; // not computed in v1
    }

    stat_keeper::PositionStats combat_stats::get_position() const {
        return position_; // not computed in v1
    }

    stat_keeper::MechanicsStats combat_stats::get_mechanics() const {
        return mech_;
    }

    stat_keeper::Summary combat_stats::get_summary() const {
        Summary s{};
        s.encounter_name = encounter_name_;
        s.duration = get_duration();
        s.rotation = get_rotation();
        s.damage = get_damage();
        s.healing = get_healing();
        s.taken = get_taken();
        s.threat = get_threat();
        s.resource = get_resource();
        s.position = get_position();
        s.mechanics = get_mechanics();

        // crude unique targets hit as unique abilities used (placeholder you can swap for per-target map)
        s.unique_targets_hit = static_cast<uint32_t>(dmg_by_ability_.size());
        return s;
    }

    // ---------- tables ----------

    template<typename Map, typename RowMaker>
    std::vector<typename std::invoke_result_t<RowMaker, typename Map::value_type>>
        combat_stats::top_n_from_map(const Map& m, size_t n, RowMaker row) {
        using Row = typename std::invoke_result_t<RowMaker, typename Map::value_type>;
        std::vector<Row> rows;
        rows.reserve(m.size());
        for (const auto& kv : m) rows.push_back(row(kv));

        std::partial_sort(rows.begin(),
            rows.begin() + std::min(n, rows.size()),
            rows.end(),
            [](const Row& a, const Row& b) { return a.total > b.total; });

        if (rows.size() > n) rows.resize(n);
        return rows;
    }

    std::vector<stat_keeper::AbilityRow>
        combat_stats::top_damage_abilities(size_t top_n) const {
        const double total = static_cast<double>(dmg_.total);
        return top_n_from_map(dmg_by_ability_, top_n, [&](const auto& kv) {
            const NamedId& ab = kv.first; const Tally& t = kv.second;
            AbilityRow r{ ab, t.total, t.hits, safe_div(t.total, static_cast<double>(std::max(1u,t.hits))),
                         safe_div(t.crits, static_cast<double>(std::max(1u,t.hits))), safe_div(t.total, total) };
            return r;
            });
    }

    std::vector<stat_keeper::AbilityRow>
        combat_stats::top_damage_taken_by_ability(size_t top_n) const {
        const double total = static_cast<double>(taken_.total_taken);
        return top_n_from_map(taken_by_ability_, top_n, [&](const auto& kv) {
            const NamedId& ab = kv.first; const Tally& t = kv.second;
            AbilityRow r{ ab, t.total, t.hits, safe_div(t.total, static_cast<double>(std::max(1u,t.hits))),
                         safe_div(t.crits, static_cast<double>(std::max(1u,t.hits))), safe_div(t.total, total) };
            return r;
            });
    }

    std::vector<stat_keeper::SourceRow>
        combat_stats::top_damage_taken_by_source(size_t top_n) const {
        struct Row { NamedId s; int64_t total; double share; };
        std::vector<Row> rows;
        double total = static_cast<double>(taken_.total_taken);
        rows.reserve(taken_by_source_.size());
        for (const auto& [src, val] : taken_by_source_) {
            rows.push_back(Row{ src, val, safe_div(val, total) });
        }
        std::partial_sort(rows.begin(), rows.begin() + std::min(top_n, rows.size()), rows.end(),
            [](const Row& a, const Row& b) { return a.total > b.total; });
        if (rows.size() > top_n) rows.resize(top_n);

        std::vector<SourceRow> out;
        out.reserve(rows.size());
        for (auto& r : rows) out.push_back(SourceRow{ r.s, r.total, r.share });
        return out;
    }

    std::vector<stat_keeper::AbilityRow>
        combat_stats::top_healing_abilities(size_t top_n) const {
        const double total = static_cast<double>(heal_.total);
        return top_n_from_map(heal_by_ability_, top_n, [&](const auto& kv) {
            const NamedId& ab = kv.first; const Tally& t = kv.second;
            AbilityRow r{ ab, t.total, t.hits, safe_div(t.total, static_cast<double>(std::max(1u,t.hits))),
                         safe_div(t.crits, static_cast<double>(std::max(1u,t.hits))), safe_div(t.total, total) };
            return r;
            });
    }

    std::vector<stat_keeper::SourceRow>
        combat_stats::top_healing_received_by_source(size_t top_n) const {
        struct Row { NamedId s; int64_t total; double share; };
        std::vector<Row> rows;
        double total = 0.0;
        for (const auto& kv : healing_received_by_source_) total += kv.second;
        rows.reserve(healing_received_by_source_.size());
        for (const auto& [src, val] : healing_received_by_source_) {
            rows.push_back(Row{ src, val, safe_div(val, total) });
        }
        std::partial_sort(rows.begin(), rows.begin() + std::min(top_n, rows.size()), rows.end(),
            [](const Row& a, const Row& b) { return a.total > b.total; });
        if (rows.size() > top_n) rows.resize(top_n);

        std::vector<SourceRow> out;
        out.reserve(rows.size());
        for (auto& r : rows) out.push_back(SourceRow{ r.s, r.total, r.share });
        return out;
    }

    std::vector<stat_keeper::UptimeRow>
        combat_stats::buff_uptimes() const {
        std::vector<UptimeRow> rows;
        const double dur_ms = static_cast<double>(get_duration().ms);
        if (dur_ms <= 0) return rows;

        // Close any still-active windows at end_ms_ for accounting
        for (const auto& [eff, st] : self_buffs_) {
            uint32_t total_ms = st.total_active_ms + (st.active ? (end_ms_ - st.active_since_ms) : 0);
            rows.push_back(UptimeRow{ eff, safe_div(total_ms, dur_ms), st.max_stacks });
        }
        std::sort(rows.begin(), rows.end(), [](const UptimeRow& a, const UptimeRow& b) { return a.uptime_pct > b.uptime_pct; });
        return rows;
    }

    std::vector<stat_keeper::UptimeRow>
        combat_stats::debuff_uptimes_on_primary_target() const {
        std::vector<UptimeRow> rows;
        const double dur_ms = static_cast<double>(get_duration().ms);
        if (dur_ms <= 0) return rows;

        for (const auto& [eff, st] : target_debuffs_) {
            uint32_t total_ms = st.total_active_ms + (st.active ? (end_ms_ - st.active_since_ms) : 0);
            rows.push_back(UptimeRow{ eff, safe_div(total_ms, dur_ms), st.max_stacks });
        }
        std::sort(rows.begin(), rows.end(), [](const UptimeRow& a, const UptimeRow& b) { return a.uptime_pct > b.uptime_pct; });
        return rows;
    }

    // ---------- burst windows ----------

    static std::optional<stat_keeper::BurstWindow>
        peak_window_impl(const std::deque<combat_stats::EventPoint>& series, uint32_t window_ms) {
        if (series.empty()) return std::nullopt;
        size_t i = 0, j = 0;
        int64_t sum = 0;
        int64_t best = 0;
        const uint32_t start_t = series.front().t;
        const uint32_t end_t = series.back().t;

        while (i < series.size()) {
            const uint32_t win_start = series[i].t;
            const uint32_t win_end = win_start + window_ms;

            while (j < series.size() && series[j].t <= win_end) { sum += series[j].v; ++j; }
            best = std::max(best, sum);

            // slide: remove i-th point and advance i
            sum -= series[i].v; ++i;
        }
        if (window_ms == 0) return std::nullopt;
        double peak = static_cast<double>(best) / (window_ms / 1000.0);
        return stat_keeper::BurstWindow{ window_ms, peak };
    }

    std::optional<stat_keeper::BurstWindow>
        combat_stats::peak_dps_window(uint32_t window_ms) const {
        return peak_window_impl(dmg_series_, window_ms);
    }
    std::optional<stat_keeper::BurstWindow>
        combat_stats::peak_hps_window(uint32_t window_ms) const {
        return peak_window_impl(heal_series_, window_ms);
    }

    // ---------- export ----------

    std::string combat_stats::to_json_summary() const {
        const auto s = get_summary();
        std::ostringstream o;
        o << std::fixed << std::setprecision(3);
        o << "{"
            << "\"encounter\":\"" << s.encounter_name << "\","
            << "\"duration_s\":" << s.duration.seconds() << ","
            << "\"apm\":" << s.rotation.apm << ","
            << "\"dps\":" << s.damage.dps << ","
            << "\"hps\":" << s.healing.hps << ","
            << "\"dtps\":" << s.taken.dtps << ","
            << "\"deaths\":" << s.taken.deaths
            << "}";
        return o.str();
    }

    std::string combat_stats::to_json_tables() const {
        auto to_rows = [](const std::vector<AbilityRow>& v) {
            std::ostringstream o; o << "[";
            for (size_t i = 0; i < v.size(); ++i) {
                const auto& r = v[i];
                o << "{\"id\":" << r.ability.id << ",\"name\":\"" << r.ability.name
                    << "\",\"total\":" << r.total << ",\"hits\":" << r.hits
                    << ",\"avg\":" << r.avg << ",\"crit\":" << r.crit_rate
                    << ",\"share\":" << r.share_pct << "}";
                if (i + 1 < v.size()) o << ",";
            }
            o << "]";
            return o.str();
            };

        std::ostringstream o; o << "{";
        o << "\"damageAbilities\":" << to_rows(top_damage_abilities(10)) << ",";
        o << "\"healingAbilities\":" << to_rows(top_healing_abilities(10));
        o << "}";
        return o.str();
    }

    // ---------- reset ----------

    void combat_stats::reset() {
        tracked_id_.reset();
        tracked_name_.clear();
        encounter_name_.clear();
        start_ms_ = end_ms_ = 0;
        encounter_open_ = false;
        rot_ = {};
        last_action_ms_ = 0;
        dmg_ = {};
        heal_ = {};
        taken_ = {};
        threat_ = {};
        resource_ = {};
        position_ = {};
        mech_ = {};
        dmg_by_ability_.clear();
        heal_by_ability_.clear();
        taken_by_ability_.clear();
        taken_by_source_.clear();
        healing_received_by_source_.clear();
        self_buffs_.clear();
        target_debuffs_.clear();
        dmg_series_.clear();
        heal_series_.clear();
    }

} // namespace swtor
