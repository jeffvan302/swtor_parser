#pragma once
#include <windows.h>
#include <psapi.h>
#include <iostream>
#include <format>
#include <locale>


/// <summary>
/// Prints the current process memory usage to the console
/// </summary>
inline void print_memory_usage() {
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    if (GetProcessMemoryInfo(GetCurrentProcess(),
        (PROCESS_MEMORY_COUNTERS*)&pmc,
        sizeof(pmc))) {
		std::cout << "    === Memory Usage ===\n";
        std::cout << "    Working Set: " << std::format(std::locale("en_US.UTF-8"), "{:L}", pmc.WorkingSetSize) << " bytes\n";
        std::cout << "    Private Bytes: " << std::format(std::locale("en_US.UTF-8"), "{:L}", pmc.PrivateUsage) << " bytes\n";
    }
    else {
        std::cerr << "GetProcessMemoryInfo failed\n";
    }
}


/// <summary>
/// Prints the current process memory usage to the console
/// </summary>
inline SIZE_T print_memory_usage(SIZE_T ref) {
	SIZE_T current = 0;
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    if (GetProcessMemoryInfo(GetCurrentProcess(),
        (PROCESS_MEMORY_COUNTERS*)&pmc,
        sizeof(pmc))) {
		current = pmc.WorkingSetSize;
		SIZE_T diff = pmc.WorkingSetSize - ref;
        std::cout << "    === Memory Usage ===\n";
        std::cout << "    Difference Size: " << std::format(std::locale("en_US.UTF-8"), "{:L}", diff) << " bytes\n";
        std::cout << "    Private Bytes: " << std::format(std::locale("en_US.UTF-8"), "{:L}", pmc.WorkingSetSize) << " bytes\n";
    }
    else {
        std::cerr << "GetProcessMemoryInfo failed\n";
    }
    return current;
}