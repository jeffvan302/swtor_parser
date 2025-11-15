# SWTOR Combat Log Parser – Integration Guide

This guide explains how to add the **SWTOR Combat Log Parser** to your project, build it on common toolchains, and use its APIs for parsing, printing, JSON round‑tripping, and event filtering.

> **What you get**
> - Allocation‑light parsing of SWTOR combat log lines into a `CombatLine` struct
> - Companion & player handling (including `@Player/Companion {typeId}:{instId}`)
> - Tolerant trailing parser: numeric values, crits, secondary values, mitigations, shields, and threat (float)
> - Pretty printing for debugging
> - Fast JSON serialize/parse (schema‑locked) with a tiny arena for `string_view` stability
> - `Events` sugar for ergonomic filtering (e.g. `(Events::ApplyEffect | Events::RemoveEffect) == line`)

---

## 1) File Layout

Add these two files to your project:

- `swtor_parser.hpp` — public API and data types
- `swtor_parser.cpp` — implementation (parser + printers + JSON + filters)

Optional: the `.cpp` contains a demo `main` guarded by `SWTOR_PARSER_DEMO_MAIN` that benchmarks a real combat file.

---

## 2) Requirements & Build Flags

- **Language**: C++20 (tested with GCC, Clang, MSVC)
- **Exceptions**: Not required by the library (no throws). You can compile with `-fno-exceptions` if desired.
- **RTTI**: Not required.

### GCC / Clang
```bash
# Release build
g++ -O3 -march=native -std=gnu++20 -c swtor_parser.cpp
ar rcs libswtor_parser.a swtor_parser.o

# With demo main (benchmark)
g++ -O3 -march=native -std=gnu++20 swtor_parser.cpp -DSWTOR_PARSER_DEMO_MAIN -o swtor_demo
```

### MSVC (Developer Command Prompt)
```bat
REM Release build
cl /O2 /std:c++20 /c swtor_parser.cpp
lib /OUT:swtor_parser.lib swtor_parser.obj

REM With demo main
cl /O2 /std:c++20 swtor_parser.cpp /DSWTOR_PARSER_DEMO_MAIN /Fe:swtor_demo.exe
```

### CMake (recommended)
```cmake
add_library(swtor_parser STATIC
  swtor_parser.cpp
)
target_include_directories(swtor_parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(swtor_parser PUBLIC cxx_std_20)

# Optional demo executable
add_executable(swtor_demo swtor_parser.cpp)
target_compile_definitions(swtor_demo PRIVATE SWTOR_PARSER_DEMO_MAIN)
target_link_libraries(swtor_demo PRIVATE swtor_parser)
```

---

## 3) Quickstart

### Include the header
```cpp
#include "swtor_parser.hpp"
```

### Parse one line
```cpp
swtor::CombatLine line{};
auto status = swtor::parse_combat_line(raw_line_sv, line);
if (status == swtor::ParseStatus::Ok) {
    // use `line`
}
```

### Pretty print for debugging
```cpp
std::string s = swtor::to_text(line, {/*multiline*/true, /*pos*/false, /*hp*/false});
std::puts(s.c_str());
```

### JSON round‑trip
```cpp
std::string json = swtor::to_json(line);

swtor::detail_json::StringArena arena; // owns strings for views
swtor::CombatLine decoded{};
bool ok = swtor::from_json(std::string_view{json}, decoded, arena);
```

### Event filtering sugar
```cpp
using namespace swtor;
if (Events::RemoveEffect == line) { /* ... */ }

// any-of set
auto effectsOrSpend = Events::Effects | Events::Spend;
if (effectsOrSpend == line) { /* ... */ }
```

---

## 4) Parsing a File In‑Memory (and Benchmarking)

Below is a reference loop that mirrors the demo `main`:

```cpp
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <chrono>
#include <iostream>
#include "swtor_parser.hpp"

int main(int argc, char** argv){
    if (argc < 2) return 1;
    const std::string path = argv[1];

    std::vector<std::string> lines;
    {
        std::ifstream in(path);
        std::string s;
        while (std::getline(in, s)) {
            if (!s.empty() && s.back()=='\r') s.pop_back();
            lines.push_back(std::move(s));
        }
    }

    size_t parsed_ok = 0;
    auto t0 = std::chrono::steady_clock::now();
    for (const auto& s : lines) {
        swtor::CombatLine L{};
        auto st = swtor::parse_combat_line(std::string_view{s}, L);
        if (st == swtor::ParseStatus::Ok) ++parsed_ok;
    }
    auto t1 = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> ms = t1 - t0;

    std::cout << "lines=" << lines.size()
              << " ok=" << parsed_ok
              << " elapsed_ms=" << ms.count()
              << " avg_ms_per_line=" << (ms.count()/lines.size())
              << "\n";
}
```

> **Performance notes**
> - The parser is branch‑light and uses `std::string_view` extensively; it does **not** allocate during parse.
> - The trailing parser supports a hot path for common cases and accepts non‑integer threat values (e.g., `<5299.0>`).
> - For multi‑threading: split the `lines` vector into contiguous blocks and parse in worker threads—no shared state.

---

## 5) API Reference (Essentials)

### Core structs
- `CombatLine`
  - `TimeStamp t { uint32_t combat_ms; int64_t refined_epoch_ms; }`
  - `EntityRef source, target` — source/target entities
  - `NamedId ability` — ability name + `{id}` if present
  - `EventEffect evt` — event kind + optional effect name/id
  - `Trailing tail` — values, mitigations, and threat

- `EntityRef`
  - `name` — canonical (no leading `@`, companion right‑side only)
  - `is_player`, `is_companion`
  - `owner` (for companions): `name_no_at`, `player_numeric_id`, `has_owner`
  - `id {hi, lo, kind}` — numeric comparison friendly

- `EventKind`
  - `ApplyEffect, RemoveEffect, Event, Spend, Restore, ModifyCharges, Unknown`

- `Trailing`
  - `kind` — `None`, `Numeric`, `Charges`, or `Unknown`
  - `val` (when `Numeric`): amount, crit, secondary, school `{name,id}`, mitigations, shield detail
  - `has_threat`, `threat` (double)

### Functions
- `ParseStatus parse_combat_line(std::string_view, CombatLine&)`
- `std::string to_text(const CombatLine&, const PrintOptions& = {})`
- `std::string to_json(const CombatLine&)`
- `bool from_json(std::string_view, CombatLine&, detail_json::StringArena&)`

### Events sugar
- `namespace Events { constexpr EventTag ApplyEffect, RemoveEffect, ...; }`
- `(Events::ApplyEffect | Events::RemoveEffect) == line`  → `true` iff `line.evt.kind` is in the set

---

## 6) Parsing Semantics & Edge Cases

- **Timestamp**: `[HH:MM:SS.mmm]` → `combat_ms` (ms since midnight). Keep `refined_epoch_ms` for NTP‑refined wall time when you have it.
- **Entity**: `[@Player#12345 | (x,y,z,face) | (hp/hpmax)]` or `[@Player/Companion {type}:{inst} | ...]`.
  - Target of `=` means **self‑target** → we alias `target = source`.
  - Players: `@` is consumed (name stored without `@`); `is_player=true`.
  - Companions: `is_companion=true`, `owner.has_owner=true`, `owner.name_no_at` populated, `id.kind=NpcOrObject` with `{typeId}:{instId}`.
- **Ability**: `[Name {id}]` → `NamedId` (name may be empty; id `0` if absent).
- **Event/Effect**: `[ApplyEffect {id}: EffectName {id}]` (effect optional). Unknown kinds map to `EventKind::Unknown`.
- **Trailing** (tolerant):
  - Numeric group: `(amount[*]? [~ secondary]? [school {id}]? [-mitig ...]?)`
  - Charges: `(N charges)`
  - Threat: terminal `<number>` **float allowed**; non‑numeric tags (e.g., `<v7.0.0b>`) are **ignored** and stripped.
  - Unknown groups (e.g., `(he3001)`) → `tail.kind = Unknown`, parse still succeeds.

---

## 7) JSON Schema (Stable Contract)

The emitted JSON from `to_json()` is compact and **schema‑locked** for speed. `from_json()` accepts **only** this schema. Highlights:

```json
{
  "t_ms": 123456,
  "t_epoch": 1700000000123,        // optional
  "src": { "name": "Player", "kind":1, "id_hi":12345, "is_player":true },
  "tgt": { "name": "Player", "kind":1, "id_hi":12345, "is_player":true },
  "ability": { "name":"Force Sweep", "id": 987654 },
  "event": { "kind":"ApplyEffect", "kind_id": 111, "effect": {"name":"Bleeding", "id":222} },
  "tail": {
    "kind":"Numeric",
    "val": { "amount": 1234, "crit": true, "secondary": 200, "school": {"name":"kinetic","id":42}, "mitig": 1, "shield": {"id": 555, "absorbed": 300, "absorbed_id": 556 } },
    "threat": 5299.0
  }
}
```

> **Note**: `StringArena` *must* outlive any `CombatLine` built by `from_json()`, because fields are `string_view`s.

---

## 8) Integration Patterns

### A) Log → Parsed lines → Analytics
1. Read file into `std::vector<std::string>` (keeps backing storage stable)
2. Parse each line with `parse_combat_line()` into a `CombatLine`
3. Accumulate stats or emit JSON rows for downstream tools

### B) Parallel Parsing
- Partition `lines` by index range; each worker thread parses its chunk.
- No shared state; pre‑reserve result containers per worker if collecting.

### C) Persistence
- Store raw lines or JSON. To reload quickly, parse from JSON using `from_json()` with an arena.

### D) Pretty Debug Dumps
- Use `to_text()` for quick diffs and test expectations. You can enable positions/health via `PrintOptions`.

---

## 9) Extending the Parser

- **Event kinds**: Add names in `kind_to_cstr()` and in the string→enum mapping at JSON parse.
- **Mitigation tokens**: Extend `parse_mitigation_tail()` with new keywords.
- **Tail grammar**: The parser is tolerant. If you encounter a recurring new pattern, add recognition in `parse_value_group()` or a new fast‑path.
- **NTP refinement**: Fill `refined_epoch_ms` once you compute an offset from `combat_ms`.
- **Stable dictionaries**: If you maintain your own ability/effect numeric tables, plug them in after parse for faster downstream comparisons.

---

## 10) Troubleshooting

- **“`std::string_view` not found”**: Ensure C++17+; we require **C++20**. For MSVC set `/std:c++20`; GCC/Clang use `-std=gnu++20`.
- **Low parse rate**: Common causes are malformed input or environment newlines. Strip `\r` on Windows logs. Our parser ignores unknown trailing tags and tolerates non‑numeric `<...>`.
- **Views dangling**: `CombatLine` stores `std::string_view`s into your line storage. Don’t free or modify those strings while using the parsed structures.
- **JSON parse fails**: `from_json()` only accepts the schema produced by `to_json()`. If you change one, update the other accordingly.

---

## 11) Examples

### Filter and count RemoveEffect lines, dump first 3 as text
```cpp
size_t removed = 0; size_t shown = 0;
for (const auto& s : lines) {
    swtor::CombatLine L{};
    if (swtor::parse_combat_line(std::string_view{s}, L) == swtor::ParseStatus::Ok) {
        if (swtor::Events::RemoveEffect == L) {
            ++removed;
            if (shown < 3) {
                std::puts(swtor::to_text(L).c_str());
                ++shown;
            }
        }
    }
}
std::printf("RemoveEffect lines: %zu\n", removed);
```

### JSON round‑trip for the first valid line
```cpp
swtor::detail_json::StringArena arena;
for (const auto& s : lines) {
    swtor::CombatLine L{};
    if (swtor::parse_combat_line(std::string_view{s}, L) == swtor::ParseStatus::Ok) {
        auto j = swtor::to_json(L);
        swtor::CombatLine back{};
        if (swtor::from_json(j, back, arena)) {
            // success
        }
        break;
    }
}
```

---

## 12) License & Attribution

Internal project asset. If you export this as a standalone library: add your license header and author metadata as needed.

---

**Questions or tweaks you want pre‑wired (e.g., a `CMakeLists.txt`, GTest suite, or a parallel parse helper)?** Ping me and I’ll bolt it on. 

