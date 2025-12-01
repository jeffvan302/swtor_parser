#pragma once
#include "base_classes.h"
#include "swtor_parser.h"
#include "ntp_timekeeper_swtor.h"
#include "time_cruncher_swtor.h"
#include "combat_state.h"
#include "parse_plugin.h"

namespace swtor {

    /// <summary>
    /// Manages plugin registration and coordinates combat log parsing through registered plugins
    /// </summary>
    class plugin_manager {
    public:
        /// <summary>
        /// Constructor
        /// </summary>
        plugin_manager();
        /// <summary>
        /// Destructor
        /// </summary>
        ~plugin_manager();
        /// <summary>
        /// Register a new plugin.
        /// </summary>
        /// <param name="plugin">Shared pointer to the plugin to register</param>
        void register_plugin(parse_plugin *plugin);
        /// <summary>
        /// Process a CombatLine through all enabled plugins.
        /// </summary>
        /// <param name="line">The combat line to process</param>
        void register_plugin(std::shared_ptr<parse_plugin> plugin);
        /// <summary>
        /// Process a CombatLine through all enabled plugins.
        /// </summary>
        /// <param name="line">The combat line to process</param>
        void process_line(CombatLine& line);
        /// <summary>
        /// Parse a combat log line string and process it through all enabled plugins
        /// </summary>
        /// <param name="line">String containing raw combat log line</param>
        void process_line(std::string line);
        /// <summary>
        /// Get a plugin by its name
        /// </summary>
        /// <param name="name">Name of the plugin to retrieve</param>
        /// <returns>Shared pointer to the plugin if found, nullptr otherwise</returns>
        std::shared_ptr<parse_plugin> get_plugin_by_name(const std::string& name);
        /// <summary>
        /// Get a plugin by its unique identifier
        /// </summary>
        /// <param name="id">Unique identifier of the plugin</param>
        /// <returns>Shared pointer to the plugin if found, nullptr otherwise</returns>
        std::shared_ptr<parse_plugin> get_plugin_by_id(const uint16_t id);
        /// <summary>
        /// Reset all plugins.
        /// </summary>
        void reset_plugins();

        /// <summary>
        /// Check if currently in combat
        /// </summary>
        /// <returns>True if in combat, false otherwise</returns>
        inline bool is_in_combat() const {
            return parse_data.combat_state->is_in_combat();
        }

        /// <summary>
        /// Check if the last line is empty
        /// </summary>
        /// <returns>True if last line is empty, false otherwise</returns>
        inline bool last_line_empty() const {
            return false;
        }

        /// <summary>
        /// Get the most recently processed combat line
        /// </summary>
        /// <returns>Copy of the last combat line</returns>
        inline CombatLine get_last_line() const {
            return parse_data.last_line;
		}

        /// <summary>
        /// Get the current local time in milliseconds since epoch
        /// </summary>
        /// <returns>Time in milliseconds since epoch</returns>
        inline int64_t get_time_in_ms_epoch() const {
            return parse_data.ntp_keeper->get_LocalTime_in_epoch_ms();
		}

        /// <summary>
        /// Get the current combat state
        /// </summary>
        /// <returns>Copy of the current combat state</returns>
        inline CombatState get_combat_state() const {
            return *parse_data.combat_state;
		}

        /// <summary>
        /// Get the parse data holder containing shared parsing context
        /// </summary>
        /// <returns>Copy of the parse data holder</returns>
        inline parse_data_holder get_parse_data() const {
            return parse_data;
        }

        /// <summary>
		/// Print registered plugins for debugging
        /// </summary>
        inline void print_registered_plugins() const {
            std::cout << "Registered Plugins:\n";
            for (const auto& plugin : parse_data.plugins) {
                std::cout << "  - " << plugin->name() << " (ID: " << plugin->get_id()
                          << ", Priority: " << plugin->get_priority()
                          << ", Enabled: " << (plugin->is_enabled() ? "Yes" : "No") << ")\n";
            }
		}



    private:
        /// <summary>
        /// Container for shared parsing data
        /// </summary>
        parse_data_holder parse_data;
        /// <summary>
        /// Sort plugins by priority
        /// </summary>
        void sort();
    };
}
