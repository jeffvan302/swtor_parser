#include "plugin_sdk.h"
#include <iostream>
#include <unordered_map>
#include <string>

/// <summary>
/// Example external plugin that tracks damage dealt by each entity
/// This demonstrates how to create a plugin that can be compiled separately
/// </summary>
class DamageTrackerPlugin : public swtor::ExternalPluginBase {
public:
    DamageTrackerPlugin() = default;
    ~DamageTrackerPlugin() override = default;

    std::string name() const override {
        return "DamageTracker";
    }

    PluginInfo GetInfo() const override {
        return {
            "DamageTracker",
            "1.0.0",
            "External Plugin Developer",
            "Tracks total damage dealt by each entity during combat",
            PLUGIN_API_VERSION
        };
    }

    void ingest(swtor::parse_data_holder& parse_data, swtor::CombatLine& line) override {
        if (parse_data.combat_state->is_in_combat() != previous_in_combat_) {
            previous_in_combat_ = parse_data.combat_state->is_in_combat();
            if (previous_in_combat_) {
				std::cout << "[DamageTracker] Entered combat" << std::endl;
            }
        }

        // Only process damage events - check event type
        if (line.event.type_id != swtor::KINDID_Event) {
            return;
        }

        if (line.event.action_id != static_cast<uint64_t>(swtor::EventActionType::Damage)) {
            return;
        }



        // Track the damage
        uint64_t source_id = line.source.id;
        int64_t damage_amount = line.tail.val.amount;

        damage_totals_[source_id] += damage_amount;
        total_damage_ += damage_amount;

        // Print damage info every 10 damage events
        event_count_++;
        if (event_count_ % 10 == 0) {
            std::cout << "[DamageTracker] Total damage: " << total_damage_ 
                      << " from " << damage_totals_.size() << " entities" << std::endl;
        }
    }

    void reset() override {
        damage_totals_.clear();
        total_damage_ = 0;
        event_count_ = 0;
        std::cout << "[DamageTracker] Reset" << std::endl;
    }

    /// <summary>
    /// Custom method to get damage total for a specific entity
    /// </summary>
    int64_t GetDamageForEntity(uint64_t entity_id) const {
        auto it = damage_totals_.find(entity_id);
        return (it != damage_totals_.end()) ? it->second : 0;
    }

    /// <summary>
    /// Get total damage across all entities
    /// </summary>
    int64_t GetTotalDamage() const {
        return total_damage_;
    }

private:
    std::unordered_map<uint64_t, int64_t> damage_totals_;
    int64_t total_damage_ = 0;
    size_t event_count_ = 0;
	bool previous_in_combat_ = false;
};

// Export the plugin using the provided macro
EXPORT_PLUGIN(DamageTrackerPlugin)
