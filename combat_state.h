#pragma once
#pragma once
#include "swtor_parser.h"
#include <memory>
#include <cstdint>
#include <vector>
#include <algorithm> 
#include <windows.h>
#include <psapi.h>
#include <iostream>
#include <format>
#include <locale>

namespace swtor {
	/**
	 * @class CombatState
	 * @brief Maintains state information for combat log parsing
	 * 
	 * This class encapsulates shared state used during the parsing of combat logs.
	 * It can be extended in the future to include additional context as needed.
	 */
	class CombatState {
	public:
		/**
		 * @brief Constructor
		 */
		CombatState() = default;
		/**
		 * @brief Destructor
		 */
		~CombatState() = default;
		// Future state variables and methods can be added here as needed.

		void ParseLine(const CombatLine& line);

		inline bool is_in_combat() const { return in_combat_; }
		inline int64_t get_combat_time() const { 
			if( in_combat_) {
				return last_combat_line_time_ - last_combat_entered_;
			}
			else {
				return last_combat_exit_ - last_combat_entered_;
			}
		}

		inline std::string print_state() const {
			std::ostringstream oss;
			oss << "CombatState:\n";
			oss << "  Last Combat Line Time: " << last_combat_line_.t.print() << "\n";
			oss << "  Last Combat Line Time: " << last_combat_line_time_ << " epoch ms\n";
			oss << "  Last Source: " << last_combat_line_.source.name << "\n";
			oss << "  Last Target: " << last_combat_line_.target.name << "\n";
			oss << "  Last Ability: " << last_combat_line_.ability.name << "\n";
			oss << "  Last Event: " << last_combat_line_.event.type_name << "\n";
			oss << "  Last Action: " << last_combat_line_.event.action_name << "\n";
			oss << "  In Combat: " << (in_combat_ ? "Yes" : "No") << "\n";
			oss << "  Combat Time: " << get_combat_time() << "ms\n";
			oss << "  Last Combat Entered: " << last_combat_entered_ << "\n";
			oss << "  Last Combat Exit: " << last_combat_exit_ << "\n";
			oss << "  Last Died: " << last_died_ << "\n";
			oss << "  Died In Combat: " << (died_in_combat_ ? "Yes" : "No") << "\n";
			oss << "  All Players Dead: " << (all_players_dead ? "Yes" : "No") << "\n";
			oss << "  Dead Players Count: " << dead_players_.size() << "\n";
			oss << "  Fighting Players Count: " << fighting_players_.size() << "\n";
			oss << "  Last Area Entered: " << last_area_entered_.area.name << "\n";
			oss << "  Owner: " << owner_.name << "\n";
			oss << "  Combat Time: " << get_combat_time() << " ms\n";
			return oss.str();
		}

		inline void reset() {
			combat_state_reset();
			dead_players_.clear();
		}
		
	private:

		void combat_state_parse_entercombat(const CombatLine& line);
		inline int combat_state_players_in_fight() const;
		inline int combat_state_players_dead() const;
		inline bool combat_state_all_players_dead() const {
			if (combat_state_players_in_fight() > 1) {
				if (combat_state_players_dead() >= combat_state_players_in_fight()) return true;
			}
			else {
				if (owner_dead_) return true;
			}
			return false;
		}
		void combat_state_parse_disciplinechange(const CombatLine& line);
		void combat_state_parse_areaenter(const CombatLine& line);
		void combat_state_parse_revive(const CombatLine& line);
		void combat_state_parse_death(const CombatLine& line);
		void combat_state_parse_damage(const CombatLine& line);
		inline void combat_state_reset();
		void combat_state_parse_exitcombat(const CombatLine& line);

		bool monitor_combat_state_{ false };
		CombatLine combat_revive_line_{};
		bool in_combat_{ false }; ///< Indicates if currently in combat
		int64_t last_combat_entered_{ -1 };
		int64_t last_combat_line_time_{ -1 };
		CombatLine last_combat_line_{};
		std::chrono::system_clock::time_point last_combat_line_time_point_{};
		int64_t last_combat_exit_{ -1 };
		int64_t last_died_{ -1 };
		bool died_in_combat_{ false };
		bool all_players_dead{ false };
		std::vector<Entity> dead_players_;
		std::vector<Entity> fighting_players_;
		AreaEnteredData last_area_entered_{};
		Entity owner_{};
		bool owner_dead_{ false };

		static constexpr int64_t SAME_COMBAT_TIME_AFTER_REVIVE = 15000; // 15 seconds
	};
} // namespace swtor
