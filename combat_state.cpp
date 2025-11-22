#include "combat_state.h"



namespace swtor {
	void CombatState::combat_state_parse_entercombat(const CombatLine& line) {
		if (!in_combat_) {
			combat_state_reset();			
			in_combat_ = true;
			last_combat_entered_ = line.t.refined_epoch_ms;
			last_combat_exit_ = last_combat_entered_;
		} else if (in_combat_ && died_in_combat_ && line.source == owner_ && monitor_combat_state_) {
				int64_t time_diff = line.t.refined_epoch_ms - combat_revive_line_.t.refined_epoch_ms;
				if (time_diff < SAME_COMBAT_TIME_AFTER_REVIVE) {
					monitor_combat_state_ = false;
					died_in_combat_ = false;
				}
				else {
					combat_state_reset();
					in_combat_ = true;
					last_combat_entered_ = line.t.refined_epoch_ms;
					last_combat_exit_ = last_combat_entered_;
				}
			}
	}

	inline int CombatState::combat_state_players_dead() const {
		return dead_players_.size();
	}

	inline int CombatState::combat_state_players_in_fight() const {
		return fighting_players_.size();
	}

	void CombatState::combat_state_parse_disciplinechange(const CombatLine& line) {
		// Currently no state changes needed for discipline change
		if (in_combat_) {
			auto posit = std::find(fighting_players_.begin(), fighting_players_.end(), line.source);
			if (posit == fighting_players_.end()) {
				fighting_players_.push_back(line.source);
			}
		}
	}

	void CombatState::combat_state_parse_areaenter(const CombatLine& line) {
		owner_dead_ = false;
		in_combat_ = false;
		last_combat_entered_ = -1;
		died_in_combat_ = false;
		last_area_entered_ = line.area_entered;
		owner_ = line.source;
		dead_players_.clear();
		all_players_dead = false;
		fighting_players_.clear();
		last_died_ = -1;
		monitor_combat_state_ = false;
		all_players_dead = false;
	}

	void CombatState::combat_state_parse_revive(const CombatLine& line) {
		if (owner_.id == line.source.id) {
			owner_dead_ = false;
			monitor_combat_state_ = true;
			combat_revive_line_ = line;
			if (all_players_dead) {
				in_combat_ = false;
			}
			all_players_dead = false;
		}
		if (!dead_players_.empty()) {
			auto posit = std::find(dead_players_.begin(), dead_players_.end(), line.source);
			if (posit != dead_players_.end()) {
				dead_players_.erase(posit);
			}
		}
		all_players_dead = combat_state_all_players_dead();
	}

	void CombatState::combat_state_parse_death(const CombatLine& line) {
		if (owner_.id == line.target.id) {
			owner_dead_ = true;
			last_died_ = line.t.refined_epoch_ms;
			died_in_combat_ = true;
		}
		if (line.target.is_player) {
			auto posit = std::find(dead_players_.begin(), dead_players_.end(), line.target);
			if (posit == dead_players_.end()) {
				dead_players_.push_back(line.target);
			}
		}
		
		if (in_combat_ && combat_state_all_players_dead()) {
			all_players_dead = true;
			in_combat_ = false;
			last_combat_exit_ = line.t.refined_epoch_ms;
			monitor_combat_state_ = false;
		}
	}

	inline void CombatState::combat_state_reset() {
		died_in_combat_ = false;
		all_players_dead = true;
		in_combat_ = false;
		monitor_combat_state_ = false;
		fighting_players_.clear();
	}

	void CombatState::combat_state_parse_exitcombat(const CombatLine& line) {
		combat_state_reset();
		in_combat_ = false;
	}

	void CombatState::combat_state_parse_damage(const CombatLine& line) {
		if (in_combat_ && died_in_combat_ && line.source == owner_ && monitor_combat_state_) {
			int64_t time_diff = line.t.refined_epoch_ms - combat_revive_line_.t.refined_epoch_ms;
			if (time_diff < SAME_COMBAT_TIME_AFTER_REVIVE) {
				monitor_combat_state_ = false;
				died_in_combat_ = false;
			}
			else {
				combat_state_reset();
			}
		}
	}

	void CombatState::ParseLine(const CombatLine& line) {
		CombatState *this_engine = this;
		last_combat_line_time_ = line.t.refined_epoch_ms;
		last_combat_line_time_point_ = line.t.to_time_point();
		last_combat_line_ = line;
		if (line == EventActionType::EnterCombat) {
			combat_state_parse_entercombat(line);
			return;
		}
		if (line == EventType::AreaEntered) {
			combat_state_parse_areaenter(line);
			return;
		}
		if (line == EventActionType::Revived) {
			combat_state_parse_revive(line);
			return;
		}
		if (line == EventActionType::Death) {
			combat_state_parse_death(line);
			return;
		}
		if (line == EventActionType::Damage) {
			combat_state_parse_damage(line);
			return;
		}
		if (line == EventType::DisciplineChanged) {
			combat_state_parse_disciplinechange(line);
			return;
		}
		if (line == EventActionType::ExitCombat) {
			combat_state_parse_exitcombat(line);
			return;
		}
	}
}