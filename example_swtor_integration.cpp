#include "swtor_parser.h"
#include "ntp_timekeeper_swtor.h"
#include "time_cruncher_swtor.h"
#include "memory_helper.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <unordered_map>
#include "combat_state.h"

/**
 * Example: Integrating NTP timing with swtor_parser
 * 
 * This demonstrates how to parse SWTOR combat logs and add
 * NTP-synchronized timestamps to each CombatLine.
 */

#define ENABLE_CATALOG_PRINTS 0
#define ENABLE_COMBAT_STATE_PRINTS 0

// Helper function to read combat log file
std::vector<std::string> readLogFile(const std::string& filename) {
    std::vector<std::string> lines;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return lines;
    }
    
    lines.reserve(100000); // Pre-allocate for typical log size
    std::string line;
    while (std::getline(file, line)) {
        // Remove trailing \r if present (CRLF files)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!line.empty()) {
            lines.push_back(std::move(line));
        }
    }
    
    return lines;
}


int run_test(std::string file_name, std::vector<swtor::CombatLine> &parsed_lines){
	std::unordered_map<uint64_t, std::string> ability_cache; // Example cache for abilities
    std::unordered_map<uint64_t, std::string> event_cache; // Example cache for abilities
    std::unordered_map<uint64_t, std::string> subevent_cache; // Example cache for abilities
    std::unordered_map<uint64_t, std::string> subevent_cache2; // Example cache for abilities

    std::cout << "=== SWTOR Parser with NTP Timing ===" << std::endl;
    std::cout << std::endl;
    
    // Step 1: Initialize NTP Time Keeper
    std::cout << "Step 1: Initializing NTP synchronization..." << std::endl;
    auto ntp_keeper = std::make_shared<swtor::NTPTimeKeeper>();
    
    // Synchronize with NTP servers
    if (!ntp_keeper->synchronize()) {
        std::cerr << "Warning: Failed to synchronize with NTP servers." << std::endl;
        auto result = ntp_keeper->getLastResult();
        std::cerr << "Last error: " << result.error_message << std::endl;
        std::cerr << "Proceeding with no offset adjustment." << std::endl;
    } else {
        auto result = ntp_keeper->getLastResult();
        std::cout << "  Synchronized with: " << result.server << std::endl;
        std::cout << "  Time offset: " << result.offset_ms << " ms" << std::endl;
        std::cout << "  Round-trip time: " << result.round_trip_ms << " ms" << std::endl;
        std::cout << "  Time Zone offset: " << ntp_keeper->get_local_offset() << " ms" << std::endl;
    }

    std::cout << "  UTC Calculated time: " << ntp_keeper->getNTPTime() << std::endl;
    std::cout << "  Local Calculated time: " << ntp_keeper->getLocalTime() << std::endl;
    std::cout << "  Day Before Calculated time: " << ntp_keeper->adjust_time(ntp_keeper->getLocalTime(), -1, 0, 0, 0, 0) << std::endl;

    std::cout << std::endl;
    
    // Step 2: Create Time Cruncher
    std::cout << "Step 2: Creating Time Cruncher..." << std::endl;
    swtor::TimeCruncher cruncher(ntp_keeper, true); // Enable late arrival adjustment
    std::cout << "  ✓ Time Cruncher ready" << std::endl;
    std::cout << std::endl;
    
    
    std::string filename = "C:\\Temp\\Logs\\combat_2025-10-08_23_54_10_906141.txt"; // argv[1];
    // Step 3: Process combat log file
    if (file_name.length() > 4) {
        filename = file_name;
    }
    
    
    
    std::cout << "Step 3: Processing combat log: " << filename << std::endl;
    
    // Read file
    auto raw_lines = readLogFile(filename);
    if (raw_lines.empty()) {
        std::cerr << "No lines read from file" << std::endl;
        return 1;
    }
    
    std::cout << "  Read " << raw_lines.size() << " lines" << std::endl;
	print_memory_usage();
    // Step 4: Parse with swtor_parser
    std::cout << "Step 4: Parsing combat lines..." << std::endl;
    
    parsed_lines.reserve(raw_lines.size());
    
    size_t parse_errors = 0;
    
    const size_t total = raw_lines.size();

    auto t0 = std::chrono::steady_clock::now();

    for (const auto& raw_line : raw_lines) {
        swtor::CombatLine line;
        auto status = swtor::parse_combat_line(raw_line, line);
        
        if (status == swtor::ParseStatus::Ok) {
            parsed_lines.push_back(line);
        } else {
            parse_errors++;
        }
    }
    
    


    auto t1 = std::chrono::steady_clock::now();
    

    
    std::cout << "  Parsed: " << parsed_lines.size() << " lines" << std::endl;
    if (parse_errors > 0) {
        std::cout << "  ! Parse errors: " << parse_errors << std::endl;
    }
    std::cout << std::endl;

	

    print_memory_usage();
    
    // Step 5: Add NTP timing
    std::cout << "Step 5: Adding NTP timestamps..." << std::endl;
    auto t2 = std::chrono::steady_clock::now();
    size_t timing_processed = cruncher.processLines(parsed_lines);
    auto t3 = std::chrono::steady_clock::now();
    std::cout << "  Processed: " << timing_processed << " lines" << std::endl;
    std::cout << std::endl;
    
    print_memory_usage();

    std::cout << "Step 6: Catalog data:" << std::endl;
    for (auto& line : parsed_lines) {
        if (!event_cache.contains(line.event.type_id)) {
            event_cache[line.event.type_id] = line.event.type_name;
        }
        if (!subevent_cache.contains(line.event.action_id)) {
            if (line.event.action_id != line.ability.id && line.source.is_player) {
                subevent_cache[line.event.action_id] = line.event.action_name;
                subevent_cache2[line.event.action_id] = line.event.data;
            }
        }
        if (!ability_cache.contains(line.ability.id)) {
            ability_cache[line.ability.id] = line.ability.name;
        }
    }

    std::cout << "        Catalog complete!" << std::endl;

    print_memory_usage();

    



    if (ENABLE_CATALOG_PRINTS == 1) {
        std::cout << "Events" << std::endl;
        for (const auto& [id, name] : event_cache) {
            std::cout << "  " << name << " = " << id << "," << std::endl;
        }

        std::cout << "Sub Events" << std::endl;
        for (const auto& [id, name] : subevent_cache2) {
            std::cout << "  " << name << " = " << id << "," << std::endl;
        }
	}
    
    

    // Step 6: Show results
    std::cout << "Results (first 5 entries):" << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    
    for (size_t i = 0; i < std::min<size_t>(5, parsed_lines.size()); i++) {
        const auto& line = parsed_lines[i];
        auto combined = line.t.to_time_point();
        
        std::cout << "Line " << (i + 1) << ":" << std::endl;
        std::cout << "  Combat ms: " << line.t.print() << std::endl;
        std::cout << "  Full time: " << combined << std::endl;
        std::cout << "  Epoch ms:  " << line.t.refined_epoch_ms << std::endl;
		std::cout << "  Event:     " << line.event.type_name << std::endl;
        std::cout << "  Action:     " << line.event.action_name << std::endl;
		if (line == swtor::EventType::AreaEntered)
			std::cout << "  Area:      " << line.area_entered.area.name << std::endl;
		if (line == swtor::EventType::DisciplineChanged)
            std::cout << "  Discipline:      " << line.discipline_changed.discipline.name << std::endl;
        
        // Show source and target
        if (!line.source.name.empty()) {
            std::cout << "  Source: " << line.source.name << std::endl;
        }
        if (!line.target.name.empty()) {
            std::cout << "  Target: " << line.target.name << std::endl;
        }
        
        // Show ability if present
        if (!line.ability.name.empty()) {
            std::cout << "  Ability: " << line.ability.name << std::endl;
        }
        
        std::cout << std::endl;
    }


    
    // Step 8: Show AreaEntered events
    std::cout << "AreaEntered Events:" << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    
    int area_count = 0;
    for (size_t i = 0; i < parsed_lines.size(); i++) {
        const auto& line = parsed_lines[i];
        
        // Check if this is an AreaEntered event
        if (line.event == swtor::EventType::AreaEntered) {
			auto combined = line.t.to_time_point();
            std::cout << "  [" << (i + 1) << "] " 
                     << combined 
                     << " - " << line.area_entered.area.name
                     << " [D " << line.area_entered.difficulty.name << "]" << std::endl;
            area_count++;
        }
    }
    
    if (area_count == 0) {
        std::cout << "  (none found)" << std::endl;
    }
    std::cout << std::endl;
    
    std::cout << "====   Combat State  ====" << std::endl;

    swtor::CombatState state_engine{};
    state_engine.reset();
    auto tc4 = std::chrono::steady_clock::now();
    for (auto& line : parsed_lines) {
        state_engine.ParseLine(line);
        if (line == swtor::EventActionType::EnterCombat) {
            if (ENABLE_COMBAT_STATE_PRINTS == 1) std::cout << state_engine.print_state();
        }
        if (line == swtor::EventActionType::ExitCombat) {
            if (ENABLE_COMBAT_STATE_PRINTS == 1) std::cout << state_engine.print_state();
        }
        if (line == swtor::EventActionType::Death && line.target.is_player) {
            if (ENABLE_COMBAT_STATE_PRINTS == 1) std::cout << state_engine.print_state();
        }
        if (line == swtor::EventActionType::Revived) {
            if (ENABLE_COMBAT_STATE_PRINTS == 1) std::cout << state_engine.print_state();
        }
        if (line == swtor::EventType::AreaEntered) {
            if (ENABLE_COMBAT_STATE_PRINTS == 1) std::cout << state_engine.print_state();
        }
    }
    auto tc5 = std::chrono::steady_clock::now();
    std::cout << "====   Combat Complete  ====" << std::endl;



    std::chrono::duration<double, std::milli> ms = t1 - t0;
    std::chrono::duration<double, std::milli> ms2 = t3 - t2;
    std::chrono::duration<double, std::milli> ms3 = tc5 - tc4;
    const double total_ms = ms.count();
    const double avg_ms_per_line = total_ms / static_cast<double>(total);
    const double lines_per_sec = (total_ms > 0.0) ? (1000.0 / avg_ms_per_line) : 0.0;
    const double timing_ms = ms2.count();
    const double avg_timing_ms_per_line = timing_ms / static_cast<double>(total);
    const double timing_combat_state = ms3.count();
    const double avg_combat_state_per_line = timing_combat_state / static_cast<double>(total);
    std::cout << std::string(80, '-') << std::endl;
    std::cout << std::endl;

    // Step 7: Statistics
    auto stats = cruncher.getStatistics();
    std::cout << "Statistics:" << std::endl;
    std::cout << "  Total lines processed: " << stats.total_lines_processed << std::endl;
    std::cout << "  Area entered events:   " << stats.area_entered_count << std::endl;
    std::cout << "  Midnight rollovers:    " << stats.midnight_rollovers_detected << std::endl;
    std::cout << "  Time jumps detected:   " << stats.time_jumps_detected << std::endl;
    std::cout << "  Max late arrival:      " << stats.max_late_arrival_ms << " ms" << std::endl;
    std::cout << "  Parse Time Elapsed: " << total_ms << " ms\n";
    std::cout << "  Parse Line Time Average: " << avg_ms_per_line << " ms/line\n";
    std::cout << "  Time Adjust Average: " << avg_timing_ms_per_line << " ms/line\n";
    std::cout << "  Combat State Average: " << avg_combat_state_per_line << " ms/line\n";
    std::cout << "  Total Time per line: " << (avg_ms_per_line + avg_timing_ms_per_line + avg_combat_state_per_line) << " ms/line\n";
    std::cout << "  Parse Time Throughput: " << lines_per_sec << " lines/sec\n";

    if (stats.total_lines_processed > 0) {
        double avg = static_cast<double>(stats.total_late_arrival_adjustment_ms) /
            static_cast<double>(stats.total_lines_processed);
        std::cout << "  Avg late arrival:      " << std::fixed << std::setprecision(2)
            << avg << " ms" << std::endl;
    }
    std::cout << std::endl;


    // Step 9: Example queries
    std::cout << "Example Time-based Queries:" << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    
    // Find events in last 10 seconds
    if (!parsed_lines.empty()) {
        int64_t last_time = parsed_lines.back().t.refined_epoch_ms;
        int64_t cutoff = last_time - 10000; // 10 seconds ago
        
        size_t recent_count = 0;
        for (const auto& line : parsed_lines) {
            if (line.t.refined_epoch_ms >= cutoff) {
                recent_count++;
            }
        }
        
        std::cout << "  Events in last 10 seconds: " << recent_count << std::endl;
    }
    
    // Calculate DPS over the entire log
    if (parsed_lines.size() > 1) {
        int64_t start_time = parsed_lines.front().t.refined_epoch_ms;
        int64_t end_time = parsed_lines.back().t.refined_epoch_ms;
        double duration_seconds = (end_time - start_time) / 1000.0;
        
        std::cout << "  Total duration: " << std::fixed << std::setprecision(2) 
                 << duration_seconds << " seconds" << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "=== Processing Complete ===" << std::endl;
    std::cout << "===   Timing Reference  ===" << std::endl;
	
    //std::chrono::system_clock::time_point
    
    int64_t offset = ntp_keeper->utc_offset_to_local_ms();
    int64_t test_off = ntp_keeper->get_local_offset();
    auto offset_duration = std::chrono::milliseconds(offset);

    std::cout << "  offset = " << offset << std::endl;
    std::cout << "  test_off = " << test_off << std::endl;
    std::cout << "  offset_duration = " << offset_duration << std::endl;
    auto act_time = ntp_keeper->getNTPTime();
    
    std::cout << "  act_time = " << act_time << std::endl;
    int64_t tmpval = ntp_keeper->getNTPTimeMs();
    std::cout << "  tmpval = " << tmpval << std::endl;
    act_time = act_time + offset_duration;
    std::cout << "  act_time = " << act_time << std::endl;
    auto test_time = std::chrono::system_clock::to_time_t(act_time);
    tm gm_time{};
    gmtime_s(&gm_time, &test_time);
    swtor::TimeStamp tt{};
    tt.year = gm_time.tm_year +1900;
    tt.month = gm_time.tm_mon +1;
    tt.day = gm_time.tm_mday;
    tt.h = gm_time.tm_hour;
    tt.m = gm_time.tm_min;
    tt.s = gm_time.tm_sec;
    std::cout << "  Test time: " << tt.print() << std::endl;
    auto zero_hour = ntp_keeper->getZeroHour(act_time);
    std::cout << "  zero hour = " << zero_hour << std::endl;
    tt.update_combat_ms();
    auto new_date = ntp_keeper->adjust_time(zero_hour, tt.combat_ms);
    std::cout << "  new date = " << new_date << std::endl;
    std::cout << "===   Timing Test Done  ===" << std::endl;
        
    print_memory_usage();
	std::cout << "===   Memory Cleanup  ===" << std::endl;
    std::cout << "=== Clearing text lines" << std::endl;
    raw_lines.clear();
    print_memory_usage();
    std::cout << "=== Clearing ability and event names" << std::endl;
    ability_cache.clear();
    event_cache.clear();
    subevent_cache.clear();
    subevent_cache2.clear();
    print_memory_usage();
    std::cout << "===  ===" << std::endl;
    
    return 0;
}

int call_file_processing(std::string file_name) {
    std::cout << "===   Starting Memory  ===" << std::endl;
    print_memory_usage();
    std::vector<swtor::CombatLine> parsed_lines;
    int results = run_test(file_name, parsed_lines);
    std::cout << "===   Final Return Memory  ===" << std::endl;
    print_memory_usage();
    parsed_lines.clear();
    parsed_lines.reserve(0);
    parsed_lines.resize(0);
    std::cout << "===   Final Cleared Memory  ===" << std::endl;
    print_memory_usage();
    std::cout << "===          DONE           ===" << std::endl << std::endl << std::endl;
    return results;
}


int main(int argc, char* argv[]) {
    std::string file_name = "";
    if (argc == 2) {
        file_name = argv[1];
    }
    if (file_name.length() > 4) {
        call_file_processing(file_name);
    }
    else {
        call_file_processing("C:\\Temp\\Logs\\combat_2025-09-13_21_31_14_478033.txt");
        //call_file_processing("C:\\Temp\\Logs\\combat_2025-08-23_21_32_22_118048.txt");
        //call_file_processing("C:\\Temp\\Logs\\combat_2025-10-08_23_54_10_906141.txt");
        
    }
    std::cout << "===   Process End Memory  ===" << std::endl;
    print_memory_usage();
    std::cout << "===     Terminating!      ===" << std::endl << std::endl << std::endl;
    return 0;
}