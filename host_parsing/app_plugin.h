#pragma once
// Include all necessary headers from the core library
#include "../core/plugin_api.h"
#include <iostream>
#include <unordered_map>
#include <string>
//#include "parse_plugin.h"
//#include "swtor_parser.h"
//#include "combat_state.h"
//#include "ntp_timekeeper_swtor.h"
//#include "time_cruncher_swtor.h"

class DamageTesterPlugin : public swtor::ExternalPluginBase {
public:
    DamageTesterPlugin() = default;
    ~DamageTesterPlugin() override = default;

    std::string name() const override;

    PluginInfo GetInfo() const override;

    void ingest(swtor::parse_data_holder& parse_data, swtor::CombatLine& line) override;

    void reset() override;

    /// <summary>
    /// Custom method to get damage total for a specific entity
    /// </summary>
    int64_t GetDamageForEntity(uint64_t entity_id) const;

    /// <summary>
    /// Get total damage across all entities
    /// </summary>
    int64_t GetTotalDamage() const;

private:
    std::unordered_map<uint64_t, int64_t> damage_totals_;
    int64_t total_damage_ = 0;
    size_t event_count_ = 0;
    bool previous_in_combat_ = false;
};
