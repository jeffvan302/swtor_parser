#include "test_plugin.h"


namespace swtor {

    std::string TestPlugin::name() const {
        return "TestPlugin";
    }

    void TestPlugin::ingest(parse_data_holder& parse_data, CombatLine& line) {
        if (line == swtor::EventActionType::Damage && previous_in_combat && line.source.is_player) {
            total_damage += line.tail.val.amount;
        }
        if (line == swtor::EventActionType::Heal && previous_in_combat && line.source.is_player) {
            total_healing += line.tail.val.amount;
		}
        if (parse_data.combat_state->is_in_combat() != previous_in_combat) {
            if (parse_data.combat_state->is_in_combat()) {
				reset();
                // Entered combat
            } else {
                // Exited combat
			}
            previous_in_combat = parse_data.combat_state->is_in_combat();
        }
    }

   double TestPlugin::get_dps() const {
        int64_t combat_time_ms = 0;
        if (previous_in_combat) {
            combat_time_ms = parse_data_->combat_state->get_combat_time();
        }
        if (combat_time_ms > 0) {
            return static_cast<double>(total_damage) / (static_cast<double>(combat_time_ms) / 1000.0);
        }
        return 0.0;
   }

    void TestPlugin::reset()  {
        // Clear statistics
        total_damage = 0;
        total_healing = 0;
        previous_in_combat = false;
    }

}