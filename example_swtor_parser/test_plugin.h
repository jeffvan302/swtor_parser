#pragma once
#include "../core/parse_manager.h"

namespace swtor {

    /// <summary>
    /// Test plugin that tracks damage and healing statistics
    /// </summary>
    class TestPlugin : public parse_plugin {
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
        /// Get the calculated damage per second
        /// </summary>
        /// <returns>Damage per second as double</returns>
        double get_dps() const;
		/// <summary>
		/// Get the calculated healing per second
		/// </summary>
		/// <returns>Healing per second as double</returns>
		double get_hps() const;
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


    private:
        /// <summary>
        /// Total damage accumulated
        /// </summary>
        int64_t total_damage{ 0 };
        /// <summary>
        /// Total healing accumulated
        /// </summary>
        int64_t total_healing{ 0 };
        /// <summary>
        /// Previous combat state
        /// </summary>
        bool previous_in_combat = false;
        /// <summary>
        /// Pointer to parse data holder
        /// </summary>
        parse_data_holder* parse_data_;
    };
}
