#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <optional>

// Minimal, self-contained parser demo for SWTOR combat logs.
// C++20, header-only API surface with implementation in swtor_parser.cpp.
// No anonymization. Player hashes are parsed as 64-bit and printed verbatim.

namespace swtor {

struct TimeStamp {
  int hh=0, mm=0, ss=0, ms=0;
};

struct EntityRef {
  std::string name;            // UTF-8 as written in the log
  bool is_player = false;
  bool is_companion = false;
  uint64_t hash = 0;           // 64-bit player/actor hash after '#'
  uint64_t static_id = 0;      // for printing id[...] (1=player, 2=companion, 3=npc as a simple convention)
  uint64_t instance_id = 0;
};

struct NamedId {
  std::string name;
  uint64_t id = 0;
};

struct EventEffect {
  std::string name;  // e.g., "ApplyEffect"
  std::string detail;// e.g., "Damage"
  uint64_t id = 0;   // the numeric effect id after {...}
};

struct Value {
  double amount = 0.0;     // numeric at end of line in <...> if present
  bool has_amount = false;
  std::string school;      // e.g., "energy"
};

struct Position {
  double x=0, y=0, z=0, facing=0;
  bool has_source_pos=false;
  bool has_target_pos=false;
};

struct CombatLine {
  TimeStamp ts;
  EntityRef source;
  EntityRef target;
  NamedId ability;      // [Ability {id}:Target |(...)] (simple parse)
  NamedId target_named; // target with {id} if present
  NamedId aux;          // e.g., [Shocked {id}]
  EventEffect event;    // [ApplyEffect {id}: Damage {id2}] (simplified)
  Value value;
  Position pos;
  bool target_is_self = false;
  std::string raw;      // original line (for debugging)
};

struct PrintOptions {
  bool show_positions = false;
};

// Parse a single combat log line into CombatLine. Returns true on success.
bool parse_line(std::string_view sv, CombatLine& out);

// Format a CombatLine similar to your sample "src: ..." snippet
std::string to_text(const CombatLine& line, const PrintOptions& opt = {});

// Convenience: parse timestamp like [hh:mm:ss.mmm]
bool parse_timestamp(std::string_view sv, TimeStamp& ts);

// Helpers (exposed for tests)
uint64_t parse_u64(std::string_view sv, bool& ok);
double   parse_double(std::string_view sv, bool& ok);

} // namespace swtor
