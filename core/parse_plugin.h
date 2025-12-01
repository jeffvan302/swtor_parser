#pragma once
#include "base_classes.h"
#include <cstdint>
#include <string>
#include <string_view>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <iostream>
#include <format>
#include <locale>
#include "swtor_parser.h"
#include "ntp_timekeeper_swtor.h"
#include "time_cruncher_swtor.h"
#include "combat_state.h"

namespace swtor {


	/// <summary>
	/// Container for shared data used during combat log parsing
	/// </summary>
	struct parse_data_holder {
	public:
		/// <summary>
		/// Shared pointer to NTP time keeper for timestamp synchronization
		/// </summary>
		std::shared_ptr<NTPTimeKeeper> ntp_keeper;
		/// <summary>
		/// Pointer to time cruncher for processing timestamps
		/// </summary>
		TimeCruncher* time_cruncher;
		/// <summary>
		/// Pointer to combat state tracker
		/// </summary>
		CombatState* combat_state;
		/// <summary>
		/// Pointer to entity manager for tracking combat participants
		/// </summary>
		EntityManager* entities;
		/// <summary>
		/// Collection of registered plugins
		/// </summary>
		std::vector<std::shared_ptr<parse_plugin>> plugins;
		/// <summary>
		/// Pointer to plugin manager
		/// </summary>
		plugin_manager* manager{ nullptr };
		/// <summary>
		/// The most recently parsed combat line
		/// </summary>
		CombatLine last_line{};
		/// <summary>
		/// The most recently parsed area enter event line
		/// </summary>
		CombatLine last_area_enter{};
		/// <summary>
		/// The most recently parsed enter combat event line
		/// </summary>
		CombatLine last_enter_combat{};
	};

	/// <summary>
	/// Base interface for combat log parsing plugins
	/// </summary>
	struct parse_plugin {
	public:
		virtual ~parse_plugin() = default;

		/// <summary>
		/// Returns the name of the plugin.
		/// </summary>
		/// <returns></returns>
		virtual std::string name() const = 0;

		/// <summary>
		/// Unique identifier for the plugin and is assigned during registration.
		/// </summary>
		uint16_t id{ 0 }; // Unique plugin ID.

		/// <summary>
		/// Priority of the plugin (lower values indicate higher priority).  Negative values are ignored and are used internally.
		/// </summary>
		int priority = 0; // Lower values indicate higher priority.

		/// <summary>
		/// Set the priority of the plugin. Any value less than zero is ignored.
		/// </summary>
		/// <param name="p"></param>
		virtual inline void set_priority(int p) { priority = p; }

		/// <summary>
		/// Get the priority of the plugin.
		/// </summary>
		/// <returns></returns>
		virtual inline int get_priority() const { return priority; }


		/// <summary>
		/// Enable the plugin.
		/// </summary>
		virtual inline void enable() { enabled = true; }
		/// <summary>
		/// Disable the plugin.
		/// </summary>
		virtual inline void disable() { enabled = false; }


		/// <summary>
		/// Get the enabled state of the plugin.
		/// </summary>
		/// <returns></returns>
		virtual inline bool is_enabled() const { return enabled; }


		/// <summary>
		/// Set the unique identifier for the plugin.
		/// </summary>
		/// <param name="parse_data">Parse data holder containing shared parsing context</param>
		/// <param name="plugin_id">Unique identifier to assign to this plugin</param>
		virtual inline void set_id(parse_data_holder &parse_data, uint16_t plugin_id) { id = plugin_id; }

		/// <summary>
		/// get the unique identifier for the plugin.
		/// </summary>
		/// <returns></returns>
		virtual inline uint16_t get_id() const { return id; }



		/// <summary>
		/// Enabled state of the plugin.  false = disabled, and will not be called during parsing.
		/// </summary>
		bool enabled{ true };

		/// <summary>
		/// Parse lines in the plugin through ingest method.
		/// </summary>
		/// <param name="parse_data">Parse data holder containing shared parsing context</param>
		/// <param name="line">The combat line to process</param>
		virtual void ingest(parse_data_holder& parse_data, CombatLine& line) = 0;

		/// <summary>
		/// Clear all state.
		/// </summary>
		virtual void reset() = 0;
	};

}
