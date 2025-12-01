#include "parse_manager.h"

namespace swtor {

    plugin_manager::plugin_manager() {
        parse_data.ntp_keeper = std::make_shared<swtor::NTPTimeKeeper>();
        parse_data.time_cruncher = new swtor::TimeCruncher(parse_data.ntp_keeper, true);
        parse_data.combat_state = new CombatState();
		parse_data.entities = new EntityManager();
        parse_data.combat_state->reset();
		parse_data.manager = this;
    }

    plugin_manager::~plugin_manager() {

    }

    void plugin_manager::register_plugin(parse_plugin* plugin) {
        std::shared_ptr<parse_plugin> plugin_ptr(plugin);
		register_plugin(plugin_ptr);
    }

    void plugin_manager::register_plugin(std::shared_ptr<parse_plugin> plugin) {
		uint16_t new_id = static_cast<uint16_t>(parse_data.plugins.size() + 1);
		plugin->set_id(parse_data, new_id);
        parse_data.plugins.push_back(plugin);
        sort();
    }

    void plugin_manager::process_line(CombatLine& line) {
        parse_data.time_cruncher->processLine(line);
        parse_data.combat_state->ParseLine(line);
		parse_data.entities->combat_state_update(parse_data.combat_state->is_in_combat());
		parse_data.entities->ParseLine(line);
        if (line == EventType::AreaEntered) {
			reset_plugins();
            parse_data.last_area_enter = line;
        }
        if (line == EventActionType::EnterCombat) {
            parse_data.last_enter_combat = line;
        }
        for (auto& plugin : parse_data.plugins) {
            if (plugin->is_enabled() && plugin->get_priority()>=0) {
				plugin->ingest(parse_data, line);
            }
        }
		parse_data.last_line = line;
        if (line == EventType::AreaEntered) {
            parse_data.last_area_enter = line;
        }
        if (line == EventActionType::EnterCombat) {
            parse_data.last_enter_combat = line;
        }
    }

    void plugin_manager::process_line(std::string str_line) {
        swtor::CombatLine line;
        auto status = swtor::parse_combat_line(str_line, line);
		process_line(line);
    }

    void plugin_manager::reset_plugins() {
        for (auto& plugin : parse_data.plugins) {
			plugin->reset();
        }
        sort();
    }
    std::shared_ptr<parse_plugin> plugin_manager::get_plugin_by_name(const std::string& name) {
        auto it = std::find_if(parse_data.plugins.begin(), parse_data.plugins.end(),
            [&name](const std::shared_ptr<parse_plugin>& plugin) {
                return plugin && plugin->name() == name;
            });

        return (it != parse_data.plugins.end()) ? *it : nullptr;
    }
    std::shared_ptr<parse_plugin> plugin_manager::get_plugin_by_id(const uint16_t id) {
        auto it = std::find_if(parse_data.plugins.begin(), parse_data.plugins.end(),
            [id](const std::shared_ptr<parse_plugin>& plugin) {
                return plugin && plugin->get_id() == id;
            });

        return (it != parse_data.plugins.end()) ? *it : nullptr;
    }

    void plugin_manager::sort() {
        std::sort(parse_data.plugins.begin(), parse_data.plugins.end(),
            [](const std::shared_ptr<parse_plugin>& a, const std::shared_ptr<parse_plugin>& b) {
                return a->get_priority() < b->get_priority();
            });
    }
}