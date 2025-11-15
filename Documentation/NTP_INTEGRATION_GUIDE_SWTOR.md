# NTP Timing Integration Guide for swtor_parser

## Overview

This guide shows how to integrate NTP timing with your existing `swtor_parser` code. The NTP timing system fills in the `refined_epoch_ms` field in your `TimeStamp` structure with accurate, date-aware timestamps.

## Quick Integration (5 Minutes)

### 1. Add Files to Your Project

Add these three files to your project alongside `swtor_parser.h` and `swtor_parser.cpp`:

```
ntp_timekeeper_swtor.h
ntp_timekeeper_swtor.cpp
time_cruncher_swtor.h
time_cruncher_swtor.cpp
```

### 2. Update Your Build System

**CMakeLists.txt:**
```cmake
add_library(swtor_parser STATIC
    swtor_parser.cpp
    swtor_parser.h
    ntp_timekeeper_swtor.cpp
    ntp_timekeeper_swtor.h
    time_cruncher_swtor.cpp
    time_cruncher_swtor.h
)

# Link pthread for NTP networking
target_link_libraries(swtor_parser PUBLIC pthread)
```

**Makefile:**
```makefile
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2
LDFLAGS = -lpthread

swtor_parser.o: swtor_parser.cpp swtor_parser.h
	$(CXX) $(CXXFLAGS) -c swtor_parser.cpp

ntp_timekeeper_swtor.o: ntp_timekeeper_swtor.cpp ntp_timekeeper_swtor.h
	$(CXX) $(CXXFLAGS) -c ntp_timekeeper_swtor.cpp

time_cruncher_swtor.o: time_cruncher_swtor.cpp time_cruncher_swtor.h
	$(CXX) $(CXXFLAGS) -c time_cruncher_swtor.cpp

your_app: your_app.cpp swtor_parser.o ntp_timekeeper_swtor.o time_cruncher_swtor.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
```

### 3. Update Your Code

**Before** (your current code):
```cpp
#include "swtor_parser.h"
#include <vector>

int main() {
    std::vector<std::string> lines = readFile("combat.txt");
    std::vector<swtor::CombatLine> parsed;
    
    for (const auto& line : lines) {
        swtor::CombatLine cl;
        if (swtor::parse_combat_line(line, cl) == swtor::ParseStatus::Ok) {
            parsed.push_back(cl);
            // cl.t.combat_ms has time, but cl.t.refined_epoch_ms is -1
        }
    }
}
```

**After** (with NTP timing):
```cpp
#include "swtor_parser.h"
#include "ntp_timekeeper_swtor.h"
#include "time_cruncher_swtor.h"
#include <vector>
#include <memory>

int main() {
    // Initialize NTP keeper (once per application)
    auto ntp = std::make_shared<swtor::NTPTimeKeeper>();
    ntp->synchronize();
    
    // Create time cruncher (once per log file)
    swtor::TimeCruncher cruncher(ntp);
    
    std::vector<std::string> lines = readFile("combat.txt");
    std::vector<swtor::CombatLine> parsed;
    
    for (const auto& line : lines) {
        swtor::CombatLine cl;
        if (swtor::parse_combat_line(line, cl) == swtor::ParseStatus::Ok) {
            // Process timing - THIS IS NEW
            cruncher.processLine(cl);
            // Now cl.t.refined_epoch_ms has the full timestamp!
            
            parsed.push_back(cl);
        }
    }
}
```

That's it! Now every `CombatLine` has `refined_epoch_ms` set.

## Understanding Your TimeStamp Structure

Your existing `TimeStamp` structure (from `swtor_parser.h`):

```cpp
struct TimeStamp {
    uint32_t combat_ms{ 0 };          // HH:MM:SS.mmm → ms since midnight
    int64_t  refined_epoch_ms{ -1 };  // fill later with NTP/refinement
};
```

- **`combat_ms`**: Already parsed by `swtor_parser` from `[HH:MM:SS.mmm]` format. This is milliseconds since midnight (0-86399999).
- **`refined_epoch_ms`**: Initially `-1`. The `TimeCruncher` fills this with the complete epoch timestamp in milliseconds.

## Common Integration Patterns

### Pattern 1: Single File Processing

```cpp
void processLogFile(const std::string& filename) {
    // One-time NTP sync
    auto ntp = std::make_shared<swtor::NTPTimeKeeper>();
    if (!ntp->synchronize()) {
        std::cerr << "Warning: NTP sync failed\n";
    }
    
    // Create cruncher for this file
    swtor::TimeCruncher cruncher(ntp);
    
    // Parse and process
    std::vector<std::string> lines = readFile(filename);
    std::vector<swtor::CombatLine> parsed;
    
    for (const auto& line : lines) {
        swtor::CombatLine cl;
        if (swtor::parse_combat_line(line, cl) == swtor::ParseStatus::Ok) {
            cruncher.processLine(cl);
            parsed.push_back(cl);
        }
    }
    
    // Use parsed lines with timestamps
    analyzeData(parsed);
}
```

### Pattern 2: Multiple Files (Share NTP)

```cpp
class LogProcessor {
private:
    std::shared_ptr<swtor::NTPTimeKeeper> ntp_;
    
public:
    LogProcessor() {
        ntp_ = std::make_shared<swtor::NTPTimeKeeper>();
        ntp_->synchronize(); // Only once!
    }
    
    void processFile(const std::string& filename) {
        // Each file gets its own cruncher
        swtor::TimeCruncher cruncher(ntp_);
        
        std::vector<std::string> lines = readFile(filename);
        for (const auto& line : lines) {
            swtor::CombatLine cl;
            if (swtor::parse_combat_line(line, cl) == swtor::ParseStatus::Ok) {
                cruncher.processLine(cl);
                handleLine(cl);
            }
        }
    }
};
```

### Pattern 3: Real-time Log Tailing

```cpp
void tailLogFile(const std::string& filename) {
    auto ntp = std::make_shared<swtor::NTPTimeKeeper>();
    ntp->synchronize();
    
    swtor::TimeCruncher cruncher(ntp);
    
    // Open file and seek to end
    std::ifstream file(filename);
    file.seekg(0, std::ios::end);
    
    std::string line;
    while (running) {
        if (std::getline(file, line)) {
            swtor::CombatLine cl;
            if (swtor::parse_combat_line(line, cl) == swtor::ParseStatus::Ok) {
                cruncher.processLine(cl);
                
                // Process in real-time
                if (isDamageEvent(cl)) {
                    updateDPSMeter(cl);
                }
            }
        } else {
            // Wait for more data
            file.clear();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}
```

### Pattern 4: Batch Processing with Statistics

```cpp
void batchProcess(const std::vector<std::string>& lines) {
    auto ntp = std::make_shared<swtor::NTPTimeKeeper>();
    ntp->synchronize();
    
    swtor::TimeCruncher cruncher(ntp);
    std::vector<swtor::CombatLine> parsed;
    
    // Parse all lines first
    for (const auto& line : lines) {
        swtor::CombatLine cl;
        if (swtor::parse_combat_line(line, cl) == swtor::ParseStatus::Ok) {
            parsed.push_back(cl);
        }
    }
    
    // Process timing in batch
    cruncher.processLines(parsed);
    
    // Show statistics
    auto stats = cruncher.getStatistics();
    std::cout << "Processed: " << stats.total_lines_processed << "\n";
    std::cout << "AreaEntered events: " << stats.area_entered_count << "\n";
    std::cout << "Midnight rollovers: " << stats.midnight_rollovers_detected << "\n";
}
```

## Using the Timestamps

### Converting to Readable Format

```cpp
// Get a CombatLine with refined_epoch_ms set
swtor::CombatLine cl = /* ... */;

// Convert to readable time
auto time = swtor::CombinedTime::fromEpochMs(cl.t.refined_epoch_ms);

std::cout << time.toString() << std::endl;          // "2025-11-11 23:54:32"
std::cout << time.toISOString() << std::endl;      // "2025-11-11T23:54:32.862Z"
std::cout << time.toString("%H:%M:%S") << std::endl; // "23:54:32"
```

### Time-based Queries

```cpp
// Find all events in the last 30 seconds
int64_t now = ntp->getNTPTimeMs();
int64_t cutoff = now - 30000;

for (const auto& line : parsed_lines) {
    if (line.t.refined_epoch_ms >= cutoff) {
        // Recent event
    }
}

// Calculate time between events
int64_t time_diff_ms = line2.t.refined_epoch_ms - line1.t.refined_epoch_ms;
double seconds = time_diff_ms / 1000.0;

// Calculate DPS over a time window
int64_t start_ms = parsed_lines.front().t.refined_epoch_ms;
int64_t end_ms = parsed_lines.back().t.refined_epoch_ms;
double duration = (end_ms - start_ms) / 1000.0;
double dps = total_damage / duration;
```

### Filtering by Event Type with Timing

```cpp
using namespace swtor;

// Find damage events in the last minute
int64_t cutoff = ntp->getNTPTimeMs() - 60000;

for (const auto& line : parsed_lines) {
    if (line.t.refined_epoch_ms >= cutoff) {
        if (Evt::EventDamage == line) {
            processDamage(line);
        }
    }
}
```

## Advanced Features

### Handling Multiple Sessions

If processing logs that span multiple play sessions:

```cpp
void processMultiSessionLog(const std::vector<std::string>& lines) {
    auto ntp = std::make_shared<swtor::NTPTimeKeeper>();
    ntp->synchronize();
    
    swtor::TimeCruncher cruncher(ntp);
    
    for (const auto& line : lines) {
        swtor::CombatLine cl;
        if (swtor::parse_combat_line(line, cl) == swtor::ParseStatus::Ok) {
            cruncher.processLine(cl);
            
            // AreaEntered events automatically reset the base time
            // No manual intervention needed!
        }
    }
}
```

### Detecting Time Gaps

```cpp
void detectGaps(const std::vector<swtor::CombatLine>& lines) {
    for (size_t i = 1; i < lines.size(); i++) {
        int64_t gap = lines[i].t.refined_epoch_ms - lines[i-1].t.refined_epoch_ms;
        
        if (gap > 300000) { // 5 minutes
            auto time = swtor::CombinedTime::fromEpochMs(lines[i].t.refined_epoch_ms);
            std::cout << "Large gap at " << time.toString() 
                     << ": " << (gap / 1000) << " seconds\n";
        }
    }
}
```

### Custom NTP Servers

```cpp
// Use specific NTP servers
std::vector<std::string> servers = {
    "time.example.com",
    "ntp.yourserver.com"
};

auto ntp = std::make_shared<swtor::NTPTimeKeeper>(servers);
```

## Error Handling

### NTP Synchronization Failures

```cpp
auto ntp = std::make_shared<swtor::NTPTimeKeeper>();

if (!ntp->synchronize()) {
    auto result = ntp->getLastResult();
    std::cerr << "NTP sync failed: " << result.error_message << "\n";
    std::cerr << "Server: " << result.server << "\n";
    
    // Continue anyway - will use local time without offset
    // This is acceptable for most use cases
}
```

### Checking Statistics for Issues

```cpp
auto stats = cruncher.getStatistics();

if (stats.midnight_rollovers_detected > 10) {
    std::cout << "Warning: Many midnight rollovers detected\n";
}

if (stats.max_late_arrival_ms > 3000) {
    std::cout << "Warning: Some entries arrived very late: " 
             << stats.max_late_arrival_ms << "ms\n";
}

if (stats.area_entered_count == 0 && stats.total_lines_processed > 1000) {
    std::cout << "Warning: No AreaEntered events found\n";
}
```

## Performance Considerations

- **NTP Sync**: ~100ms, done once per application
- **Per-line Processing**: ~1-2 microseconds
- **Memory**: ~1KB per TimeCruncher
- **Thread Safety**: NTPTimeKeeper is thread-safe, TimeCruncher is not

**Threading Pattern:**
```cpp
// One shared NTP keeper
auto ntp = std::make_shared<swtor::NTPTimeKeeper>();
ntp->synchronize();

// Each thread gets its own cruncher
#pragma omp parallel for
for (int i = 0; i < files.size(); i++) {
    swtor::TimeCruncher cruncher(ntp); // Thread-local
    processFile(files[i], cruncher);
}
```

## Testing Your Integration

### Test 1: Verify Timestamps Are Set

```cpp
swtor::CombatLine cl;
swtor::parse_combat_line(test_line, cl);
cruncher.processLine(cl);

assert(cl.t.refined_epoch_ms != -1); // Should no longer be -1
assert(cl.t.combat_ms > 0);          // Should have combat_ms
```

### Test 2: Verify Midnight Rollover

Create a test file with:
```
[23:59:58.000] ...
[23:59:59.000] ...
[00:00:00.000] ...
[00:00:01.000] ...
```

Then check:
```cpp
auto stats = cruncher.getStatistics();
assert(stats.midnight_rollovers_detected == 1);
```

### Test 3: Verify AreaEntered Anchoring

```cpp
// Process lines including AreaEntered
cruncher.processLines(parsed_lines);

auto stats = cruncher.getStatistics();
std::cout << "Found " << stats.area_entered_count << " anchors\n";
```

## Troubleshooting

### Issue: refined_epoch_ms is still -1

**Solution:** Make sure you call `cruncher.processLine(cl)` after parsing:
```cpp
swtor::parse_combat_line(line, cl);
cruncher.processLine(cl);  // Don't forget this!
```

### Issue: Timestamps seem wrong

**Check:**
1. Is NTP synchronized? `ntp->isSynchronized()`
2. What's the offset? `ntp->getOffsetMs()`
3. Check statistics: `cruncher.getStatistics()`

### Issue: Build errors

**Common fixes:**
- Ensure C++20: `-std=c++20`
- Link pthread: `-lpthread`
- Include all files in build

## Complete Example

See `example_swtor_integration.cpp` for a complete, working example that:
- Initializes NTP
- Reads a combat log
- Parses with `swtor_parser`
- Adds timestamps with `TimeCruncher`
- Shows results and statistics

**Build and run:**
```bash
g++ -std=c++20 -O2 \
    swtor_parser.cpp \
    ntp_timekeeper_swtor.cpp \
    time_cruncher_swtor.cpp \
    example_swtor_integration.cpp \
    -o example -lpthread

./example combat_2025-10-08_23_54_10_906141.txt
```

## Next Steps

1. Build the example program
2. Test with your combat logs
3. Integrate into your application
4. Check statistics for any issues
5. Use `refined_epoch_ms` for time-based analysis

## API Quick Reference

```cpp
// NTPTimeKeeper
auto ntp = std::make_shared<swtor::NTPTimeKeeper>();
ntp->synchronize();                  // Sync with NTP
int64_t offset = ntp->getOffsetMs(); // Get time offset
bool synced = ntp->isSynchronized(); // Check if synced
int64_t now = ntp->getNTPTimeMs();   // Get current NTP time

// TimeCruncher
swtor::TimeCruncher cruncher(ntp);
cruncher.processLine(line);          // Process one line
cruncher.processLines(lines);        // Process many lines
auto stats = cruncher.getStatistics(); // Get statistics
cruncher.reset();                    // Reset for new file

// CombinedTime
auto time = swtor::CombinedTime::fromEpochMs(cl.t.refined_epoch_ms);
std::string iso = time.toISOString();
std::string fmt = time.toString("%Y-%m-%d %H:%M:%S");
int64_t ms = time.toEpochMs();
```

## Support

If you encounter issues:
1. Check this guide
2. Run the example program
3. Check statistics output
4. Verify NTP synchronization
5. Review your build configuration
