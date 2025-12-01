#include "timing_plugin.h"


namespace swtor {

    std::string TimingPlugin::name() const {
        return "TimingPlugin";
    }

    void TimingPlugin::ingest(parse_data_holder& parse_data, CombatLine& line) {

        if (last_time_ms_ == 0) {
            last_time_ms_ = line.t.refined_epoch_ms;
            previous_in_combat = parse_data.combat_state->is_in_combat();
            return;
		}

        if (parse_data.combat_state->is_in_combat() != previous_in_combat) {
            previous_in_combat = parse_data.combat_state->is_in_combat();
        }
        if (previous_in_combat && speed_factor_in_combat_) {
            // In combat speed adjustment
            int64_t delta_ms = line.t.refined_epoch_ms - last_time_ms_;
            int64_t adjusted_ms = static_cast<int64_t>(static_cast<float>(delta_ms) / speed_factor_);
            Sleep(adjusted_ms);
        }
        if (!previous_in_combat && speed_factor_out_of_combat_) {
            // Out of combat speed adjustment
            int64_t delta_ms = line.t.refined_epoch_ms - last_time_ms_;
            int64_t adjusted_ms = static_cast<int64_t>(static_cast<float>(delta_ms) / speed_factor_);
            Sleep(adjusted_ms);
		}
		last_time_ms_ = line.t.refined_epoch_ms;
    }


    void TimingPlugin::reset() {
        // Clear statistics
        previous_in_combat = false;
        last_time_ms_ = 0;
    }

}