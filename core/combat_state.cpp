#include "combat_state.h"



namespace swtor {

	std::shared_ptr<EntityState> EntityManager::entity(uint64_t id) {
		for(int pos = 0; pos < entities_.size(); pos++) {
			if( entities_[pos]->id == id) {
				return entities_[pos];
			}
		}
		return nullptr;
	}

	std::shared_ptr<EntityState> EntityManager::entity(Entity& ent) {
		auto it = std::find_if(entities_.begin(), entities_.end(),
			[&ent](const std::shared_ptr<EntityState>& es) { return es->id == ent.id; });
		if (it != entities_.end()) {
			return *it;
		}
		else {
			auto new_entity_state = std::make_shared<EntityState>(ent);
			entities_.push_back(new_entity_state);
			return new_entity_state;
		}
	}

	EntityState::EntityState() {
		Effects.reserve(64);
		AppliedBy.reserve(64);

	}

	std::shared_ptr<EntityState> EntityManager::owner(){
		for(int pos = 0; pos < entities_.size(); pos++) {
			if( entities_[pos]->owner) {
				return entities_[pos];
			}
		}
		return nullptr;
	}

	void EntityManager::new_combat_reset(){
		for (int pos = (entities_.size() - 1); pos >= 0; pos--) {
			if (!entities_[pos]->is_player() && !entities_[pos]->is_companion()) {
				entities_.erase(entities_.begin() + pos);
			}
			else {
				entities_[pos]->target_owner = nullptr;
				entities_[pos]->death_count = 0;
				entities_[pos]->revive_count = 0;
				entities_[pos]->total_damage_taken = 0;
				entities_[pos]->total_healing_taken = 0;
				entities_[pos]->total_damage_done = 0;
				entities_[pos]->total_healing_done = 0;
				entities_[pos]->total_overheal_done = 0;
				entities_[pos]->total_absorb_done = 0;
				entities_[pos]->total_threat = 0;
				entities_[pos]->total_shielding_done = 0;
				entities_[pos]->total_defect_done = 0;
				entities_[pos]->total_glance_done = 0;
				entities_[pos]->total_dodge_done = 0;
				entities_[pos]->total_parry_done = 0;
				entities_[pos]->total_resist_done = 0;
				entities_[pos]->total_miss_done = 0;
				entities_[pos]->total_immune_done = 0;
			}
		}
	}

	EntityManager::EntityManager() {
		reset();
	}

	void EntityManager::ParseLine(CombatLine& line) {
		
		if (line == EventType::AreaEntered) {
			reset();
		}

		std::shared_ptr<EntityState> source = nullptr;
		std::shared_ptr<EntityState> target = nullptr;

		if (entities_.empty()) {
			source = std::make_shared<EntityState>(line.source);
			if (!line.target.empty) 
				if (line.source.id != line.target.id)
					target = std::make_shared<EntityState>(line.target);

			entities_.push_back(source);
			if (target != nullptr)
				entities_.push_back(target);
		}
		else {
			for(int pos = 0; pos < entities_.size(); pos++) {
				if( entities_[pos]->id == line.source.id) {
					source = entities_[pos];
				}
				if( entities_[pos]->id == line.target.id) {
					target = entities_[pos];
				}
			}
			if( source == nullptr) {
				source = std::make_shared<EntityState>(line.source);
				entities_.push_back(source);
			}
			if (!line.target.empty && target == nullptr)
				if (line.source.id != line.target.id) {
					target = std::make_shared<EntityState>(line.target);
					entities_.push_back(target);
				}
		}
		if (source != nullptr) {
			source->entity = line.source;
		}
		if (target != nullptr) {
			target->entity = line.target;
		}
		if (line == EventActionType::Death) {
			if (target != nullptr) {
				target->is_dead = true;
				target->death_count += 1;
			}
		}
		if (line == EventActionType::Revived) {
			if (source != nullptr) {
				source->is_dead = false;
				source->revive_count += 1;
			}
		}
		if (line == EventActionType::Damage) {
			if (source != nullptr) {
				source->total_damage_done += line.tail.val.amount;
				source->total_threat += static_cast<uint64_t>(line.tail.threat);
			}
			if (target != nullptr) {
				target->total_damage_taken += line.tail.val.amount;
			}
		}
		if (line == EventActionType::Heal) {
			if (source != nullptr) {
				source->total_healing_done += line.tail.val.amount;
				source->total_overheal_done += line.tail.val.has_secondary ? line.tail.val.secondary : 0;
			}
			if (target != nullptr) {
				target->total_healing_taken += line.tail.val.amount;
			}
		}
		if (line == EventActionType::ModifyThreat) {
			if (source != nullptr) {
				source->total_threat += static_cast<uint64_t>(line.tail.threat);
			}
		}

		if (line == EventType::AreaEntered) {
			if (source != nullptr) {
				source->owner = true;
			}
		}

		if (line == EventActionType::TargetSet) {
			// Currently no state changes needed for energy changes
			if (source != nullptr && target != nullptr) {
				source->target = line.target;
				source->target_owner = target;
			}
		}

		if (line == EventActionType::TargetCleared) {
			// Currently no state changes needed for energy changes
			if (source != nullptr) {
				source->target = empty_entity_;
				source->target_owner = nullptr;
			}
		}
		if (line.tail.kind != ValueKind::None) {
			switch (line.tail.val.mitig) {
				case MitigationFlags::Shield:
					if (source != nullptr) {
						source->total_shielding_done++;
						source->total_absorb_done += line.tail.val.shield.absorbed;
					}
					break;
				case MitigationFlags::Deflect:
					if (source != nullptr) {
						source->total_defect_done++;
					}
					break;
				case MitigationFlags::Glance:
					if (source != nullptr) {
						source->total_glance_done++;
					}
					break;
				case MitigationFlags::Dodge:
					if (source != nullptr) {
						source->total_dodge_done++;
					}
					break;
				case MitigationFlags::Parry:
					if (source != nullptr) {
						source->total_parry_done++;
					}
					break;
				case MitigationFlags::Resist:
					if (source != nullptr) {
						source->total_resist_done++;
					}
					break;
				case MitigationFlags::Miss:
					if (source != nullptr) {
						source->total_miss_done++;
					}
					break;
				case MitigationFlags::Immune:
					if (source != nullptr) {
						source->total_immune_done++;
					}
					break;
				default:
					break;
			}
		}
		if (line == EventType::ApplyEffect && line != EventActionType::Damage && line != EventActionType::Heal && target != nullptr) {
			bool tfound = false;
			bool sfound = false;
			for(int pos = 0; pos < target->Effects.size(); pos++) {
				if( *(target->Effects[pos]) == line) {
					// Effect already applied
					tfound = true;
					target->Effects[pos]->update(line);
					break;
				}
			}
			if (!tfound) {
				auto new_effect = std::make_shared<Applied_Effect>(line);
				target->Effects.push_back(new_effect);
			}
			else {

			}
			for (int pos = 0; pos < source->AppliedBy.size(); pos++) {
				if (*(source->AppliedBy[pos]) == line) {
					// Effect already applied
					sfound = true;
					source->AppliedBy[pos]->update(line);
					break;
				}
			}
			if (!sfound) {
				auto new_effect = std::make_shared<Applied_Effect>(line);
				target->AppliedBy.push_back(new_effect);
			}

		}
		if (line == EventType::RemoveEffect && line != EventActionType::Damage && line != EventActionType::Heal && target != nullptr) {
			for (int pos = (target->Effects.size() - 1); pos >= 0; pos--) {
				if( *(target->Effects[pos]) == line) {
					// Remove effect
					target->Effects.erase(target->Effects.begin() + pos);
					
				}
			}
			for (int pos = (source->AppliedBy.size() - 1); pos >= 0; pos--) {
				if (*(source->AppliedBy[pos]) == line) {
					// Remove effect
					source->AppliedBy.erase(source->AppliedBy.begin() + pos);

				}
			}
			return;
		}
		if (line == EventType::ModifyCharges && line != EventActionType::Damage && line != EventActionType::Heal && target != nullptr) {
			for (int pos = 0; pos < target->Effects.size(); pos++) {
				if( *(target->Effects[pos]) == line) {
					// Modify charges
					target->Effects[pos]->update(line);
				}
			}
			for (int pos = 0; pos < source->AppliedBy.size(); pos++) {
				if (*(source->AppliedBy[pos]) == line) {
					// Modify charges
					source->AppliedBy[pos]->update(line);
				}
			}
			return;
		}

	}

	inline void CombatState::combat_state_parse_entercombat(const CombatLine& line) {
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

	inline void CombatState::combat_state_parse_disciplinechange(const CombatLine& line) {
		// Currently no state changes needed for discipline change
		if (in_combat_) {
			auto posit = std::find(fighting_players_.begin(), fighting_players_.end(), line.source);
			if (posit == fighting_players_.end()) {
				fighting_players_.push_back(line.source);
			}
		}
	}

	inline void CombatState::combat_state_parse_areaenter(const CombatLine& line) {
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

	inline void CombatState::combat_state_parse_revive(const CombatLine& line) {
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

	inline void CombatState::combat_state_parse_death(const CombatLine& line) {
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

	inline void CombatState::combat_state_parse_exitcombat(const CombatLine& line) {
		combat_state_reset();
		in_combat_ = false;
	}

	inline void CombatState::combat_state_parse_damage(const CombatLine& line) {
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

	void CombatState::ParseLine(CombatLine& line) {
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