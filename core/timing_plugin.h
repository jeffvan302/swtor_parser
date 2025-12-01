#pragma once
#include "parse_manager.h"

namespace swtor {

    /// <summary>
    /// Plugin that manages timing and speed factor adjustments for combat log playback
    /// </summary>
    class TimingPlugin : public parse_plugin {
    public:
        /// <summary>
        /// Returns the name of the plugin
        /// </summary>
        /// <returns>Plugin name as string</returns>
        std::string name() const override;

        /// <summary>
        /// Process a combat line through the plugin
        /// </summary>
        /// <param name="parse_data">Parse data holder containing shared parsing context</param>
        /// <param name="line">The combat line to process</param>
        void ingest(parse_data_holder& parse_data, CombatLine& line) override;

        /// <summary>
        /// Set the unique identifier for the plugin.
        /// </summary>
        /// <param name="parse_data">Parse data holder containing shared parsing context</param>
        /// <param name="plugin_id">Unique identifier to assign to this plugin</param>
        virtual inline void set_id(parse_data_holder& parse_data, uint16_t plugin_id) override {
            id = plugin_id;
            parse_data_ = &parse_data;
        }

        /// <summary>
        /// Reset all plugin state
        /// </summary>
        void reset() override;

        /// <summary>
        /// Set the speed factor for playback
        /// </summary>
        /// <param name="factor">Speed multiplier (1.0 = normal speed)</param>
        inline void set_speed_factor(float factor) {
            speed_factor_ = factor;
		}

        /// <summary>
        /// Enable or disable speed factor while in combat
        /// </summary>
        /// <param name="enable">True to enable speed factor in combat</param>
        inline void set_speed_factor_in_combat(bool enable) {
            speed_factor_in_combat_ = enable;
        }

        /// <summary>
        /// Enable or disable speed factor while out of combat
        /// </summary>
        /// <param name="enable">True to enable speed factor out of combat</param>
        inline void set_speed_factor_out_of_combat(bool enable) {
            speed_factor_out_of_combat_ = enable;
		}

    private:
		/// <summary>
		/// Speed multiplier for playback
		/// </summary>
		float speed_factor_{ 1.0f };
		/// <summary>
		/// Whether to apply speed factor during combat
		/// </summary>
		bool speed_factor_in_combat_{ true };
		/// <summary>
		/// Whether to apply speed factor outside of combat
		/// </summary>
		bool speed_factor_out_of_combat_{ false };
        /// <summary>
        /// Previous combat state
        /// </summary>
        bool previous_in_combat = false;
		/// <summary>
		/// Last processed timestamp in milliseconds
		/// </summary>
		int64_t last_time_ms_{ 0 };
        /// <summary>
        /// Pointer to parse data holder
        /// </summary>
        parse_data_holder* parse_data_;
    };
}
