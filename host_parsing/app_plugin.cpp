#include "app_plugin.h"


/// <summary>
/// Example external plugin that tracks damage dealt by each entity
/// This demonstrates how to create a plugin that can be compiled separately
/// </summary>
PluginInfo DamageTesterPlugin::GetInfo() const {
    return {
        "DamageTester",
        "1.0.0",
        "External Plugin Developer",
        "Tracks total damage dealt by each entity during combat",
        PLUGIN_API_VERSION
    };
}

std::string DamageTesterPlugin::name() const {
    return "DamageTester";
}

void DamageTesterPlugin::ingest(swtor::parse_data_holder& parse_data, swtor::CombatLine& line) {
    if (parse_data.combat_state->is_in_combat() != previous_in_combat_) {
        previous_in_combat_ = parse_data.combat_state->is_in_combat();
        if (previous_in_combat_) {
            std::cout << "[DamageTester] Entered combat" << std::endl;
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
        std::cout << "[DamageTester] Total damage: " << total_damage_
            << " from " << damage_totals_.size() << " entities" << std::endl;
    }
}

void DamageTesterPlugin::reset() {
    damage_totals_.clear();
    total_damage_ = 0;
    event_count_ = 0;
    std::cout << "[DamageTester] Reset" << std::endl;
}

/// <summary>
/// Custom method to get damage total for a specific entity
/// </summary>
int64_t DamageTesterPlugin::GetDamageForEntity(uint64_t entity_id) const {
    auto it = damage_totals_.find(entity_id);
    return (it != damage_totals_.end()) ? it->second : 0;
}

/// <summary>
/// Get total damage across all entities
/// </summary>
int64_t DamageTesterPlugin::GetTotalDamage() const {
    return total_damage_;
}


// Export the plugin using the provided macro
EXPORT_PLUGIN(DamageTesterPlugin)
