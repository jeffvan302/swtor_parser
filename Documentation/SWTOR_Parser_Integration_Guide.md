
# SWTOR Parser — Integration Guide

> Version: 1.0 • Language: C++20 • Namespace: `swtor`

This guide explains how to integrate the **SWTOR Parser** (C++ library) into your own project. It covers setup, APIs, usage patterns, performance tips, memory management, and troubleshooting. The examples assume you’re using the latest `swtor_parser.h` / `swtor_parser.cpp` pair provided in this project.

---

## 1) What the library does

- Parses **Star Wars™: The Old Republic** combat log lines into a strongly-typed `swtor::CombatLine` struct.
- Canonicalizes **entities** (players, companions, NPCs) into numeric IDs for fast comparisons.
- Extracts **event kind**, **ability/effect IDs**, **mitigation flags**, **positions**, **health**, and **threat/value** data.
- Provides **pretty-printing** and **JSON** round‑trip (`to_text`, `to_json_text`, `from_json_text`).
- Offers **event tags and matchers** to make filtering ergonomic (e.g., `if (Events::Damage == line)`).
- Includes **high‑performance parsing** with a vectorized fast path and a tiny formatter for logging.
- Implements **explicit memory management helpers** to clone or dispose `CombatLine` objects when building higher‑level systems that cache or forward lines selectively.

---

## 2) Project layout

Add these two files to your project:

```
swtor_parser.h
swtor_parser.cpp
```

> **Important:** They must be compiled together (same C++ standard options), or you’ll get unresolved externals for functions like `to_text` or `to_json_text`.

---

## 3) Build requirements

- **C++ standard:** C++20 (`/std:c++20` on MSVC, `-std=c++20` on GCC/Clang).
- **Warnings:** Recommended (MSVC) `/W4 /WX-` or (GCC/Clang) `-Wall -Wextra -Wpedantic`.
- **Optimization:** `-O2` (GCC/Clang) or `/O2` (MSVC) for release builds.
- **Character set:** Use UTF‑8 (MSVC: `/utf-8`). Logs can contain non-ASCII names.
- **Runtime:** No additional dependencies; uses only the C++ standard library.

### CMake example

```cmake
cmake_minimum_required(VERSION 3.20)
project(swtor_consumer CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(swtor_parser STATIC
    swtor_parser.cpp
    swtor_parser.h
)

add_executable(demo main.cpp)
target_link_libraries(demo PRIVATE swtor_parser)
target_compile_definitions(demo PRIVATE SWTOR_PARSER_DEMO_MAIN=1)
```

---

## 4) Quick start

### 4.1 Read lines into memory and parse

```cpp
#include "swtor_parser.h"
#include <fstream>
#include <vector>
#include <string>
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: demo <path-to-combat-log.txt>\n";
        return 1;
    }
    const std::string path = argv[1];

    // Read whole file in memory (vector of lines)
    std::ifstream in(path, std::ios::binary);
    if (!in) { std::cerr << "failed to open file\n"; return 1; }
    std::vector<std::string> lines;
    lines.reserve(1<<20);
    std::string s;
    while (std::getline(in, s)) {
        if (!s.empty() && s.back() == '\r') s.pop_back(); // CRLF -> LF
        lines.emplace_back(std::move(s));
    }

    // Parse
    swtor::Parser p;
    std::vector<swtor::CombatLine> parsed;
    parsed.reserve(lines.size());

    for (const auto& L : lines) {
        swtor::CombatLine out{};
        if (p.parse_line(L, out)) {
            parsed.emplace_back(std::move(out));
        }
    }

    std::cout << "Parsed " << parsed.size() << " / " << lines.size() << " lines\n";
}
```

### 4.2 Pretty-print one parsed line

```cpp
const auto text = swtor::to_text(parsed.front(), swtor::PrintOptions{});
std::cout << text << "\n";
```

### 4.3 JSON round‑trip

```cpp
const std::string json = swtor::to_json_text(parsed[123]);
swtor::CombatLine cloned{};
if (swtor::from_json_text(json, cloned)) {
    // cloned now equals parsed[123] (field-wise)
}
```

---

## 5) Filtering with event tags and matchers

The library exposes a concise API to filter lines by **event kind** and other properties.

```cpp
using swtor::Events;

// Quick predicate-style checks:
if (Events::Damage == line) { /* handle damage */ }
if (Events::Heal   == line) { /* handle heal   */ }
if (Events::Apply  == line) { /* any ApplyEffect */ }
if (Events::Remove == line) { /* any RemoveEffect */ }

// Combined tags using bitwise OR:
auto dmg_or_heal = Events::Damage | Events::Heal;
if (dmg_or_heal == line) { /* fast-path */ }

// Match specific effect ID(s):
if (swtor::match_effect(line, /*Effect ID*/ 836045448945501ULL /* Damage */)) {
    // damage across any ability
}

// Match by ability ID:
if (swtor::match_ability(line, /*Ability ID*/ 801281983643648ULL /* Explosive Surge */)) {
    // all events sourced from that ability
}
```

> Internally, `Events::Damage`, `Events::Heal`, etc. map to a compact `EventTag`/`EventSet` that checks `line.event.kind` and (optionally) canonical effect IDs.


---

## 6) Entities: players, companions, NPCs

- Players start with **`@`** and include a **`#`** hash: `@Name#68844...`  
- Companions follow **`owner/companion`** naming, e.g.:  
  `@Pugzeronine#.../Z0-0M {3841216886079488}:6168012297889`  
  The parser sets `EntityRef::is_player` and `EntityRef::is_companion` booleans accordingly, and stores **static** and **instance** numeric IDs when present.
- NPCs: `Name {StaticId}:InstanceId`

You can compare entities via **numeric IDs** for speed:

```cpp
if (line.source.id == line.target.id) {
    // self-target ([=] in raw log)
}
```

---

## 7) Memory management: clone / dispose

C++ requires explicit management when you **retain** parsed objects beyond the current scope.  
The parser returns `CombatLine` as a **POD-like** struct without owning external memory, so simple copies are cheap. The library also exposes **helpers**:

```cpp
// Deep clone (future-proof if internals gain owned buffers):
swtor::CombatLine* heap_copy = swtor::clone(line);

// ... use it in caches or cross-thread queues ...

swtor::dispose(heap_copy);  // deletes and nulls (safe no-op on nullptr)
```

Guidelines:

- **Default:** keep `CombatLine` by value in vectors (fast & compact).  
- **Clone only when needed** (e.g., delayed processing queues, ring buffers).  
- **Dispose** cloned instances when the consumer is done.  
- The struct is trivially movable; prefer `std::vector<CombatLine>` over `new[]`.

---

## 8) High‑performance parsing

The parser employs several techniques to maximize throughput:

- **`std::string_view` slicing** to avoid transient allocations.
- A **vectorized fast path** for common delimiters (`[]`, `{}`, `()`, `|`, `:`) using byte‑wise scanning.
- **Tiny formatter** for fast logging without `std::ostringstream` overhead.
- Branch‑reduced parsers for timestamps and integers (IDs).

### Batch parsing pattern

```cpp
std::vector<swtor::CombatLine> out;
out.resize(lines.size());

size_t ok = 0;
for (size_t i = 0; i < lines.size(); ++i) {
    ok += p.parse_line(lines[i], out[i]) ? 1 : 0;
}
```

### Threading

- The `Parser` has no global state; you can **shard** work across threads (e.g., by line ranges).  
- Avoid sharing mutable `Parser` across threads; create one per worker.  
- Use **producer/consumer** queues if you need to stream-parse while ingesting.

---

## 9) Pretty print & JSON

### 9.1 `to_text`

Produces a compact, human‑readable representation useful for debugging. You can tweak `PrintOptions` (e.g., show IDs, numeric only, width limits).

```cpp
swtor::PrintOptions opts;
opts.show_coordinates = true;
opts.show_health = true;
std::cout << swtor::to_text(line, opts) << "\n";
```

### 9.2 `to_json_text`

Serializes all relevant fields, including sub‑structures like `source`, `target`, `ability`, `effect`, `timestamp`, `mitigation`, `threat/value`, etc.

```cpp
std::string json = swtor::to_json_text(line);
```

### 9.3 `from_json_text`

```cpp
swtor::CombatLine x{};
if (swtor::from_json_text(json, x)) {
    // valid
}
```

#### JSON schema (simplified)

```json
{
  "timestamp": { "hh": 8, "mm": 42, "ss": 13, "ms": 127 },
  "source": { "name": "Pugzeronine", "is_player": true, "is_companion": false, "static_id": 0, "instance_id": 0, "hash": 688451045458733 },
  "target": { "name": "Operations Training Dummy", "is_player": false, "is_companion": false, "static_id": 2857785339412480, "instance_id": 8475000173833 },
  "ability": { "name": "Sever Force", "id": 813226287693824 },
  "event":   { "kind": "ApplyEffect", "id": 836045448945477, "effect_name": "Damage", "effect_id": 836045448945501 },
  "value":   { "amount": 4728, "modifiers": ["critical?"], "mitigation_flags": 0 },
  "position": { "x": 9.82, "y": 9.25, "z": 16.52, "facing": 2.92 }
}
```

> Actual keys can vary slightly; consult the current header for exact names.

---

## 10) Working with enums and event sets

Use `EventKind`, `EventTag`, and `EventSet` to match quickly:

```cpp
switch (line.event.kind) {
    case swtor::EventKind::ApplyEffect:
        if (line.event.effect_id == swtor::EffectId::Damage) { /* ... */ }
        break;
    case swtor::EventKind::RemoveEffect:
        // ...
        break;
    default: break;
}

// Shorthand via operator== with tags
if ((Events::Apply | Events::Remove) == line) {
    // any effect boundary
}
```

---

## 11) End‑to‑end example (filter + JSON dump)

```cpp
std::vector<std::string> damage_json;
damage_json.reserve(parsed.size());

for (const auto& line : parsed) {
    if (swtor::Events::Damage == line) {
        damage_json.emplace_back(swtor::to_json_text(line));
    }
}
```

---

## 12) Performance tips & pitfalls

- **Reserve** vectors based on file size or line count to avoid reallocations.
- Normalize line endings (`\r\n` → `\n`) to keep the fast path hot.
- Reuse `Parser` across many lines to benefit from branch‑predictor warmth.
- Avoid `std::regex`. The parser does not use it.
- Use **release** builds for throughput tests; debug builds are much slower.
- If you see **`std::string_view` not found**, ensure **C++20** is enabled for *all translation units*.
- If you see an **unresolved external** for `to_text`/`to_json_text`, make sure `swtor_parser.cpp` is compiled and linked into your target.

---

## 13) Error handling

- `Parser::parse_line(...)` returns `bool` (success).
- On failure, `out` is left in a known-empty state; you can log the raw line for diagnostics.
- For telemetry, you can count failures and sample raw lines for later analysis.

---

## 14) Extending the parser

- Add mappings for **new event kinds** introduced by SWTOR updates (IDs are stable across locales).
- Extend the **mitigation flags** as new markers appear (absorb, dodge, reflect, immune).
- Add new helpers (e.g., `match_source(hash)`, `match_target_static(static_id)`).

> The code is designed to keep the hot path minimal; put optional features behind fast checks.

---

## 15) Demo main (optional)

If you compile a console demo with `SWTOR_PARSER_DEMO_MAIN` defined, the provided `main()` will:

- Accept a **file path** argument.
- Load the file into memory.
- Parse every line while timing total and per‑line average.
- Print basic statistics.

Enable with a preprocessor definition on the demo target:
- MSVC: **`/DSWTOR_PARSER_DEMO_MAIN`**
- GCC/Clang: **`-DSWTOR_PARSER_DEMO_MAIN`**

---

## 16) Troubleshooting

### Q: *“`std` has no member `string_view`”*  
A: You are compiling with an older language standard or a TU is not using C++20. Set `/std:c++20` or `-std=c++20` for **all** files.

### Q: *Linker error: unresolved external for `to_text`*  
A: Ensure `swtor_parser.cpp` is part of the build **and** compiled with the same standard switches.

### Q: *ICE / constexpr complaint on `operator|`*  
A: Some MSVC versions had issues with `constexpr` operators on enum‑class flags. Use the provided header (guards applied), or update to a recent toolset (VS 2022 17.10+).

### Q: *Non‑ASCII names look garbled*  
A: Force UTF‑8 source and runtime: MSVC `/utf-8`. Ensure your console font supports the characters.

---

## 17) FAQ

- **Can I keep every `CombatLine` by value?** Yes; it’s compact and movable. Clone only for selective caching and dispose when done.
- **How do I detect self‑target (`[=]`)?** `line.target_is_self` or compare `line.source.id == line.target.id`.
- **Companions?** The parser sets `is_companion = true` and fills `owner` sub‑info when `Owner/Companion {Static}:Instance` appears.

---

## 18) License

Include your project’s chosen license here, or keep these files internal to your application.

---

## 19) Appendix — Minimal interfaces (for reference)

```cpp
namespace swtor {
  struct Parser {
    bool parse_line(std::string_view raw, CombatLine& out) const noexcept;
  };

  struct CombatLine {
    TimeStamp ts;             // hh:mm:ss.ms
    EntityRef source, target; // IDs + flags (player/companion)
    NamedId  ability;         // name + id
    EventEffect event;        // kind + effect id/name
    Value     value;          // amount + flags
    Position  pos;            // xyz + facing
    bool      target_is_self; // [=] shorthand
    // ... other fields as in header
  };

  std::string to_text(const CombatLine&, const PrintOptions& = {});
  std::string to_json_text(const CombatLine&);
  bool        from_json_text(std::string_view, CombatLine&);

  // Memory helpers:
  CombatLine* clone(const CombatLine&);
  void        dispose(CombatLine*&); // deletes and nulls
}
```

Happy parsing!
