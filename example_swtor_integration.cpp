#include "swtor_parser.h"
#include "ntp_timekeeper_swtor.h"
#include "time_cruncher_swtor.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>

/**
 * Example: Integrating NTP timing with swtor_parser
 * 
 * This demonstrates how to parse SWTOR combat logs and add
 * NTP-synchronized timestamps to each CombatLine.
 */

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

int main(int argc, char* argv[]) {
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
        std::cout << "  ✓ Synchronized with: " << result.server << std::endl;
        std::cout << "  ✓ Time offset: " << result.offset_ms << " ms" << std::endl;
        std::cout << "  ✓ Round-trip time: " << result.round_trip_ms << " ms" << std::endl;
    }
    std::cout << std::endl;
    
    // Step 2: Create Time Cruncher
    std::cout << "Step 2: Creating Time Cruncher..." << std::endl;
    swtor::TimeCruncher cruncher(ntp_keeper, true); // Enable late arrival adjustment
    std::cout << "  ✓ Time Cruncher ready" << std::endl;
    std::cout << std::endl;
    
    
    std::string filename = "C:\\Temp\\Logs\\combat_2025-09-13_21_31_14_478033.txt"; // argv[1];
    // Step 3: Process combat log file
    if (argc == 2) {
		filename = argv[1];
    }
    
    
    std::cout << "Step 3: Processing combat log: " << filename << std::endl;
    
    // Read file
    auto raw_lines = readLogFile(filename);
    if (raw_lines.empty()) {
        std::cerr << "No lines read from file" << std::endl;
        return 1;
    }
    
    std::cout << "  Read " << raw_lines.size() << " lines" << std::endl;
    
    // Step 4: Parse with swtor_parser
    std::cout << "Step 4: Parsing combat lines..." << std::endl;
    std::vector<swtor::CombatLine> parsed_lines;
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

    

    
    // Step 5: Add NTP timing
    std::cout << "Step 5: Adding NTP timestamps..." << std::endl;
    auto t2 = std::chrono::steady_clock::now();
    size_t timing_processed = cruncher.processLines(parsed_lines);
    auto t3 = std::chrono::steady_clock::now();
    std::cout << "  Processed: " << timing_processed << " lines" << std::endl;
    std::cout << std::endl;
    
    // Step 6: Show results
    std::cout << "Results (first 5 entries):" << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    
    for (size_t i = 0; i < std::min<size_t>(5, parsed_lines.size()); i++) {
        const auto& line = parsed_lines[i];
        auto combined = swtor::CombinedTime::fromEpochMs(line.t.refined_epoch_ms);
        
        std::cout << "Line " << (i + 1) << ":" << std::endl;
        std::cout << "  Combat ms: " << line.t.combat_ms << std::endl;
        std::cout << "  Full time: " << combined.toString() << std::endl;
        std::cout << "  ISO 8601:  " << combined.toISOString() << std::endl;
        std::cout << "  Epoch ms:  " << line.t.refined_epoch_ms << std::endl;
		std::cout << "  Event:     " << line.evt.effect.name << std::endl;
		std::cout << "  Area:      " << line.area_entered.area.name << std::endl;
        std::cout << "  Area:      " << line.discipline_changed.discipline.name << std::endl;
        
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


    std::chrono::duration<double, std::milli> ms = t1 - t0;
    std::chrono::duration<double, std::milli> ms2 = t3 - t2;
    const double total_ms = ms.count();
    const double avg_ms_per_line = total_ms / static_cast<double>(total);
    const double lines_per_sec = (total_ms > 0.0) ? (1000.0 / avg_ms_per_line) : 0.0;
	const double timing_ms = ms2.count();
	const double avg_timing_ms_per_line = timing_ms / static_cast<double>(total);

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
    std::cout << "  Total Time per line: " << (avg_ms_per_line + avg_timing_ms_per_line) << " ms/line\n";
    std::cout << "  Parse Time Throughput: " << lines_per_sec << " lines/sec\n";
    
    if (stats.total_lines_processed > 0) {
        double avg = static_cast<double>(stats.total_late_arrival_adjustment_ms) / 
                     static_cast<double>(stats.total_lines_processed);
        std::cout << "  Avg late arrival:      " << std::fixed << std::setprecision(2) 
                 << avg << " ms" << std::endl;
    }
    std::cout << std::endl;
    
    // Step 8: Show AreaEntered events
    std::cout << "AreaEntered Events:" << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    
    int area_count = 0;
    for (size_t i = 0; i < parsed_lines.size(); i++) {
        const auto& line = parsed_lines[i];
        
        // Check if this is an AreaEntered event
        if (std::string_view(line.evt.effect.name).find("AreaEntered") != std::string_view::npos) {
            auto combined = swtor::CombinedTime::fromEpochMs(line.t.refined_epoch_ms);
            std::cout << "  [" << (i + 1) << "] " 
                     << combined.toString() << " - "
                     << line.evt.effect.name << std::endl;
            area_count++;
        }
    }
    
    if (area_count == 0) {
        std::cout << "  (none found)" << std::endl;
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
    
    return 0;
}
