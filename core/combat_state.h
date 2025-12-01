#pragma once
#include "base_classes.h"
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

	
	/// <summary>
	/// Represents an applied effect (buff/debuff/DoT/HoT) on a target
	/// </summary>
	class Applied_Effect {
		public:

		/// <summary>
		/// Default constructor
		/// </summary>
		Applied_Effect() = default;
		/// <summary>
		/// Default destructor
		/// </summary>
		~Applied_Effect() = default;

		/// <summary>
		/// Constructor from combat line
		/// </summary>
		/// <param name="line">Combat line containing effect application</param>
		Applied_Effect(CombatLine& line) :
			id(line.event.action_id),
			source_id(line.source.id),
			target_id(line.target.id),
			ability_id(line.ability.id),
			charges(line.tail.charges),
			applied_time_ms(line.t.refined_epoch_ms)
		{
			applied_line = line;
		}

		/// <summary>
		/// Update effect from a new combat line
		/// </summary>
		/// <param name="line">Combat line with updated effect data</param>
		inline void update(CombatLine& line) {
			id = line.event.action_id;
			source_id = line.source.id;
			target_id = line.target.id;
			ability_id = line.ability.id;
			charges = line.tail.charges;
			applied_time_ms = line.t.refined_epoch_ms;
			applied_line = line;
		}

		/// <summary>
		/// Effect identifier
		/// </summary>
		uint64_t id{ 0 };
		/// <summary>
		/// Source entity identifier
		/// </summary>
		uint64_t source_id{ 0 };
		/// <summary>
		/// Target entity identifier
		/// </summary>
		uint64_t target_id{ 0 };
		/// <summary>
		/// Ability identifier
		/// </summary>
		uint64_t ability_id{ 0 };
		/// <summary>
		/// Number of charges/stacks
		/// </summary>
		int32_t charges{ 0 };
		/// <summary>
		/// Time effect was applied in milliseconds since epoch
		/// </summary>
		int64_t applied_time_ms{ 0 };
		/// <summary>
		/// Combat line that applied this effect
		/// </summary>
		CombatLine applied_line{};
	};

	/// <summary>
	/// Compare effect with entity
	/// </summary>
	/// <param name="p">Applied effect</param>
	/// <param name="et">Entity</param>
	/// <returns>True if target matches entity</returns>
	inline bool operator==(const Applied_Effect& p, const Entity& et) { return p.target_id == et.id; }
	/// <summary>
	/// Compare entity with effect
	/// </summary>
	/// <param name="et">Entity</param>
	/// <param name="p">Applied effect</param>
	/// <returns>True if target matches entity</returns>
	inline bool operator==(const Entity& et, const Applied_Effect& p) { return p.target_id == et.id; }
	/// <summary>
	/// Compare effect with ability
	/// </summary>
	/// <param name="p">Applied effect</param>
	/// <param name="ab">Ability identifier</param>
	/// <returns>True if ability matches</returns>
	inline bool operator==(const Applied_Effect& p, const NamedId& ab) { return p.ability_id  == ab.id; }
	/// <summary>
	/// Compare ability with effect
	/// </summary>
	/// <param name="ab">Ability identifier</param>
	/// <param name="p">Applied effect</param>
	/// <returns>True if ability matches</returns>
	inline bool operator==(const NamedId& ab, const Applied_Effect& p) { return p.ability_id == ab.id; }
	/// <summary>
	/// Compare effect with event effect
	/// </summary>
	/// <param name="p">Applied effect</param>
	/// <param name="evt">Event effect</param>
	/// <returns>True if action IDs match</returns>
	inline bool operator==(const Applied_Effect& p, const EventEffect& evt) { return p.id == evt.action_id; }
	/// <summary>
	/// Compare event effect with applied effect
	/// </summary>
	/// <param name="evt">Event effect</param>
	/// <param name="p">Applied effect</param>
	/// <returns>True if action IDs match</returns>
	inline bool operator==(const EventEffect& evt, const Applied_Effect& p) { return p.id == evt.action_id; }
	/// <summary>
	/// Compare combat line with applied effect
	/// </summary>
	/// <param name="line">Combat line</param>
	/// <param name="p">Applied effect</param>
	/// <returns>True if line matches effect identifiers</returns>
	inline bool operator==(const CombatLine& line, const Applied_Effect& p) { return (p.id == line.event.action_id && p.target_id == line.target.id && p.source_id == line.source.id); }


	/// <summary>
	/// Tracks the state and statistics for a single entity (player, companion, or NPC)
	/// </summary>
	class EntityState {
	public:

		/// <summary>
		/// Default constructor
		/// </summary>
		EntityState();
		/// <summary>
		/// Default destructor
		/// </summary>
		~EntityState() = default;

		/// <summary>
		/// Constructor from entity
		/// </summary>
		/// <param name="ent">Entity to track</param>
		EntityState(Entity ent) : id(ent.id), entity(ent) {}

		/// <summary>
		/// Get current hit points as a percentage
		/// </summary>
		/// <returns>Hit points percentage (0.0 to 100.0)</returns>
		inline float_t hitpoints_percent() const {
			if (entity.hp.max > 0) {
				return static_cast<float_t>((entity.hp.current * 100) / entity.hp.max);
			}
			return 0;
		}

		/// <summary>
		/// Get current hit points
		/// </summary>
		/// <returns>Current hit points</returns>
		inline int hit_points_current() const {
			return static_cast<int>(entity.hp.current);
		}
		/// <summary>
		/// Get maximum hit points
		/// </summary>
		/// <returns>Maximum hit points</returns>
		inline int hit_points_max() const {
			return static_cast<int>(entity.hp.max);
		}


		/// <summary>
		/// Entity identifier
		/// </summary>
		uint64_t id{ 0 };
		/// <summary>
		/// Entity data
		/// </summary>
		Entity entity{};

		/// <summary>
		/// Current target entity
		/// </summary>
		Entity target{};

		/// <summary>
		/// Shared pointer to target's entity state
		/// </summary>
		std::shared_ptr<EntityState> target_owner{ nullptr };

		/// <summary>
		/// Whether this entity is the player character
		/// </summary>
		bool owner{ false };

		/// <summary>
		/// Number of times entity has died
		/// </summary>
		int death_count{ 0 };
		/// <summary>
		/// Number of times entity has been revived
		/// </summary>
		int revive_count{ 0 };
		/// <summary>
		/// Total damage taken
		/// </summary>
		uint64_t total_damage_taken{ 0 };
		/// <summary>
		/// Total healing taken
		/// </summary>
		uint64_t total_healing_taken{ 0 };
		/// <summary>
		/// Total damage done
		/// </summary>
		uint64_t total_damage_done{ 0 };
		/// <summary>
		/// Total healing done
		/// </summary>
		uint64_t total_healing_done{ 0 };
		/// <summary>
		/// Total overheal done
		/// </summary>
		uint64_t total_overheal_done{ 0 };
		/// <summary>
		/// Total absorb shields granted
		/// </summary>
		uint64_t total_absorb_done{ 0 };
		/// <summary>
		/// Total threat generated
		/// </summary>
		uint64_t total_threat{ 0 };
		
		/// <summary>
		/// Number of attacks shielded
		/// </summary>
		uint16_t total_shielding_done{ 0 };
		/// <summary>
		/// Number of attacks deflected
		/// </summary>
		uint16_t total_defect_done{ 0 };
		/// <summary>
		/// Number of attacks dodged
		/// </summary>
		uint16_t total_dodge_done{ 0 };
		/// <summary>
		/// Number of glancing blows
		/// </summary>
		uint16_t total_glance_done{ 0 };
		/// <summary>
		/// Number of attacks parried
		/// </summary>
		uint16_t total_parry_done{ 0 };
		/// <summary>
		/// Number of attacks resisted
		/// </summary>
		uint16_t total_resist_done{ 0 };
		/// <summary>
		/// Number of attacks missed
		/// </summary>
		uint16_t total_miss_done{ 0 };
		/// <summary>
		/// Number of attacks immuned
		/// </summary>
		uint16_t total_immune_done{ 0 };

		/// <summary>
		/// Whether entity is currently dead
		/// </summary>
		bool is_dead{ false };
		/// <summary>
		/// Check if entity is a player
		/// </summary>
		/// <returns>True if player</returns>
		inline bool is_player() const { return entity.is_player; }
		/// <summary>
		/// Check if entity is a companion
		/// </summary>
		/// <returns>True if companion</returns>
		inline bool is_companion() const { return entity.is_companion; }

		/// <summary>
		/// Effects that are applied to this entity
		/// </summary>
		std::vector<std::shared_ptr<Applied_Effect>> Effects;
		/// <summary>
		/// Effects that this entity has applied to others
		/// </summary>
		std::vector<std::shared_ptr<Applied_Effect>> AppliedBy;

	};

	/// <summary>
	/// Compare entity state with entity
	/// </summary>
	/// <param name="p">Entity state</param>
	/// <param name="et">Entity</param>
	/// <returns>True if IDs match</returns>
	inline bool operator==(const EntityState& p, const Entity& et) { return p.id == et.id; }

	/// <summary>
	/// Manages all entities in combat, tracking their state and relationships
	/// </summary>
	class EntityManager {
		public:
		/// <summary>
		/// Constructor
		/// </summary>
		EntityManager();
		/// <summary>
		/// Destructor
		/// </summary>
		~EntityManager() = default;
		/// <summary>
		/// Get or create entity state by ID
		/// </summary>
		/// <param name="id">Entity identifier</param>
		/// <returns>Shared pointer to entity state</returns>
		std::shared_ptr<EntityState> entity(uint64_t id);
		/// <summary>
		/// Get or create entity state from entity object
		/// </summary>
		/// <param name="ent">Entity object</param>
		/// <returns>Shared pointer to entity state</returns>
		std::shared_ptr<EntityState> entity(Entity& ent);
		/// <summary>
		/// Get the owner (player character) entity state
		/// </summary>
		/// <returns>Shared pointer to owner entity state</returns>
		std::shared_ptr<EntityState> owner();

		/// <summary>
		/// Get all tracked entities
		/// </summary>
		/// <returns>Reference to vector of all entity states</returns>
		inline std::vector<std::shared_ptr<EntityState>>& all_entities() {
			return entities_;
		}

		/// <summary>
		/// Update combat state and reset entities if entering new combat
		/// </summary>
		/// <param name="in_combat">Whether currently in combat</param>
		inline void combat_state_update(bool in_combat) {
			if (last_combat_state != in_combat) {
				last_combat_state = in_combat;
				if (in_combat) {
					new_combat_reset();
				}
			}
		}

		/// <summary>
		/// Parse a combat line and update entity states
		/// </summary>
		/// <param name="line">Combat line to parse</param>
		void ParseLine(CombatLine& line);

		/// <summary>
		/// Reset all entity data
		/// </summary>
		inline void reset() {
			entities_.clear();
			entities_.reserve(128);
			last_combat_state = false;
		}

	private:
		/// <summary>
		/// Vector of all tracked entity states
		/// </summary>
		std::vector<std::shared_ptr<EntityState>> entities_;
		/// <summary>
		/// Previous combat state
		/// </summary>
		bool last_combat_state{ false };
		/// <summary>
		/// Empty entity placeholder
		/// </summary>
		Entity empty_entity_{};
		/// <summary>
		/// Reset entities for new combat
		/// </summary>
		void new_combat_reset();
	};

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

		/// <summary>
		/// Parse a combat line and update combat state
		/// </summary>
		/// <param name="line">Combat line to parse</param>
		virtual void ParseLine(CombatLine& line);

		/// <summary>
		/// Check if currently in combat
		/// </summary>
		/// <returns>True if in combat</returns>
		virtual inline bool is_in_combat() const { return in_combat_; }
		/// <summary>
		/// Get current combat duration in milliseconds
		/// </summary>
		/// <returns>Combat duration in milliseconds</returns>
		virtual inline int64_t get_combat_time() const {
			if( in_combat_) {
				return last_combat_line_time_ - last_combat_entered_;
			}
			else {
				return last_combat_exit_ - last_combat_entered_;
			}
		}

		/// <summary>
		/// Get the last area entered data
		/// </summary>
		/// <returns>Area entered data structure</returns>
		virtual inline AreaEnteredData get_last_area_entered() const {
			return last_area_entered_;
		}

		/// <summary>
		/// Print current combat state as formatted string
		/// </summary>
		/// <returns>Formatted state string</returns>
		virtual inline std::string print_state() const {
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

		/// <summary>
		/// Reset combat state
		/// </summary>
		virtual inline void reset() {
			combat_state_reset();
			dead_players_.clear();
		}
		
	private:

		/// <summary>
		/// Parse enter combat event
		/// </summary>
		/// <param name="line">Combat line</param>
		inline void combat_state_parse_entercombat(const CombatLine& line);
		/// <summary>
		/// Get number of players currently in fight
		/// </summary>
		/// <returns>Player count</returns>
		inline int combat_state_players_in_fight() const;
		/// <summary>
		/// Get number of dead players
		/// </summary>
		/// <returns>Dead player count</returns>
		inline int combat_state_players_dead() const;
		/// <summary>
		/// Check if all players are dead
		/// </summary>
		/// <returns>True if all players dead</returns>
		inline bool combat_state_all_players_dead() const {
			if (combat_state_players_in_fight() > 1) {
				if (combat_state_players_dead() >= combat_state_players_in_fight()) return true;
			}
			else {
				if (owner_dead_) return true;
			}
			return false;
		}
		/// <summary>
		/// Parse discipline change event
		/// </summary>
		/// <param name="line">Combat line</param>
		inline void combat_state_parse_disciplinechange(const CombatLine& line);
		/// <summary>
		/// Parse area enter event
		/// </summary>
		/// <param name="line">Combat line</param>
		inline void combat_state_parse_areaenter(const CombatLine& line);
		/// <summary>
		/// Parse revive event
		/// </summary>
		/// <param name="line">Combat line</param>
		inline void combat_state_parse_revive(const CombatLine& line);
		/// <summary>
		/// Parse death event
		/// </summary>
		/// <param name="line">Combat line</param>
		inline void combat_state_parse_death(const CombatLine& line);
		/// <summary>
		/// Parse damage event
		/// </summary>
		/// <param name="line">Combat line</param>
		inline void combat_state_parse_damage(const CombatLine& line);
		/// <summary>
		/// Reset combat state to initial values
		/// </summary>
		inline void combat_state_reset();
		/// <summary>
		/// Parse exit combat event
		/// </summary>
		/// <param name="line">Combat line</param>
		inline void combat_state_parse_exitcombat(const CombatLine& line);

		/// <summary>
		/// Whether to monitor combat state
		/// </summary>
		bool monitor_combat_state_{ false };
		/// <summary>
		/// Combat line containing revive event
		/// </summary>
		CombatLine combat_revive_line_{};
		/// <summary>
		/// Indicates if currently in combat
		/// </summary>
		bool in_combat_{ false };
		/// <summary>
		/// Time when combat was last entered
		/// </summary>
		int64_t last_combat_entered_{ -1 };
		/// <summary>
		/// Time of last combat line
		/// </summary>
		int64_t last_combat_line_time_{ -1 };
		/// <summary>
		/// Last processed combat line
		/// </summary>
		CombatLine last_combat_line_{};
		/// <summary>
		/// Time point of last combat line
		/// </summary>
		std::chrono::system_clock::time_point last_combat_line_time_point_{};
		/// <summary>
		/// Time when combat was last exited
		/// </summary>
		int64_t last_combat_exit_{ -1 };
		/// <summary>
		/// Time of last death
		/// </summary>
		int64_t last_died_{ -1 };
		/// <summary>
		/// Whether death occurred in combat
		/// </summary>
		bool died_in_combat_{ false };
		/// <summary>
		/// Whether all players are dead
		/// </summary>
		bool all_players_dead{ false };
		/// <summary>
		/// List of dead players
		/// </summary>
		std::vector<Entity> dead_players_;
		/// <summary>
		/// List of players currently fighting
		/// </summary>
		std::vector<Entity> fighting_players_;
		/// <summary>
		/// Data about last area entered
		/// </summary>
		AreaEnteredData last_area_entered_{};
		/// <summary>
		/// Owner (player character) entity
		/// </summary>
		Entity owner_{};
		/// <summary>
		/// Whether owner is dead
		/// </summary>
		bool owner_dead_{ false };

		/// <summary>
		/// Time threshold for considering combat as continuing after revive (15 seconds)
		/// </summary>
		static constexpr int64_t SAME_COMBAT_TIME_AFTER_REVIVE = 15000;
	};
} // namespace swtor
