#include <iostream>
#include <windows.h>
#include <filesystem>
#include <stdexcept>
#include <vector>
#include <string>
#include <fstream>
#include <memory>
#include <unordered_map>
#include <string_view>
#include <span>
#include <system_error>
#include "../core/parse_manager.h"
#include "app_plugin.h"

// Forward declarations for DLL functions
typedef void* (*CreateParserFunc)();
typedef void (*DestroyParserFunc)(void*);
typedef void* (*GetPluginManagerFunc)(void*);
typedef void (*LoadPluginFunc)(void*, const char*);
typedef void (*LoadPluginDirectFunc)(void*, void*);

typedef void (*ProcessLineFunc)(void*, const char*);
typedef void (*ResetPluginsFunc)(void*);

namespace fs = std::filesystem;

class DamageTesterPlugin;


fs::path GetExecutableDir()
{
    wchar_t buffer[MAX_PATH];
    DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (len == 0 || len == MAX_PATH) {
        throw std::runtime_error("GetModuleFileNameW failed");
    }

    fs::path exePath(buffer);
    return exePath.remove_filename();  // directory containing the exe
}


fs::path ResolvePluginDir(const std::string& plugin_dir)
{
    fs::path p(plugin_dir);

    // If it's already absolute, just use it.
    if (p.is_absolute()) {
        return p;
    }

    // 1) Try relative to the current working directory.
    if (fs::exists(p) && fs::is_directory(p)) {
        return fs::canonical(p);
    }

    // 2) Try relative to the executable directory.
    fs::path exeDir = GetExecutableDir();
    fs::path candidate = exeDir / p;
    if (fs::exists(candidate) && fs::is_directory(candidate)) {
        return fs::canonical(candidate);
    }

    // 3) If still not found, just return the original relative path so caller can handle it.
    return p;
}

class ParserHost {
public:
    ParserHost() = default;
    ~ParserHost() {
        Shutdown();
    }

    bool Initialize(const std::string& core_dll_path) {
        // Load core DLL
        core_dll_ = LoadLibraryA(core_dll_path.c_str());
        if (!core_dll_) {
            std::cerr << "Failed to load core DLL: " << core_dll_path << std::endl;
            return false;
        }

        // Get function pointers
        create_parser_ = reinterpret_cast<CreateParserFunc>(
            GetProcAddress(core_dll_, "CreateParser"));
        destroy_parser_ = reinterpret_cast<DestroyParserFunc>(
            GetProcAddress(core_dll_, "DestroyParser"));
        get_plugin_manager_ = reinterpret_cast<GetPluginManagerFunc>(
            GetProcAddress(core_dll_, "GetPluginManager"));
        load_plugin_ = reinterpret_cast<LoadPluginFunc>(
            GetProcAddress(core_dll_, "LoadExternalPlugin"));
        load_plugin_direct_ = reinterpret_cast<LoadPluginDirectFunc>(
            GetProcAddress(core_dll_, "LoadExternalPlugin_Direct"));
        process_line_ = reinterpret_cast<ProcessLineFunc>(
            GetProcAddress(core_dll_, "ProcessCombatLine"));
        reset_plugins_ = reinterpret_cast<ResetPluginsFunc>(
            GetProcAddress(core_dll_, "ResetAllPlugins"));

        if (!create_parser_ || !destroy_parser_) {
            std::cerr << "Failed to get required functions from core DLL" << std::endl;
            FreeLibrary(core_dll_);
            core_dll_ = nullptr;
            return false;
        }

        // Create parser instance
        parser_instance_ = create_parser_();
        if (!parser_instance_) {
            std::cerr << "Failed to create parser instance" << std::endl;
            FreeLibrary(core_dll_);
            core_dll_ = nullptr;
            return false;
        }

        main_parser = static_cast<swtor::plugin_manager*>(parser_instance_);

        std::cout << "Parser host initialized successfully" << std::endl;
        return true;
    }

    bool LoadPluginDirect(void* plugin_ptr) {
        if (!load_plugin_direct_ || !parser_instance_ || !plugin_ptr) {
            std::cerr << "Parser not initialized" << std::endl;
            return false;
        }
        load_plugin_direct_(parser_instance_, plugin_ptr);
        return true;
	}

    bool LoadPluginsFromDirectory(const std::string& plugin_dir) {
        if (!load_plugin_ || !parser_instance_) {
            std::cerr << "Parser not initialized" << std::endl;
            return false;
        }

        fs::path dir = ResolvePluginDir(plugin_dir);

        if (!fs::exists(dir) || !fs::is_directory(dir)) {
            std::cerr << "Plugin directory does not exist: " << dir << std::endl;
            std::cerr << "Original parameter was: " << plugin_dir << std::endl;
            return false;
        }

        std::cout << "Scanning for plugins in: " << dir << std::endl;
        
        int loaded_count = 0;
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".dll") {
                std::string plugin_path = entry.path().string();
                std::cout << "Loading plugin: " << plugin_path << std::endl;
                
                load_plugin_(parser_instance_, plugin_path.c_str());
                loaded_count++;
            }
        }

        std::cout << "Loaded " << loaded_count << " external plugins" << std::endl;
        return true;
    }

    void ProcessCombatLine(const std::string& line) {
        if (process_line_ && parser_instance_) {
            process_line_(parser_instance_, line.c_str());
        }
    }

    void ResetPlugins() {
        if (reset_plugins_ && parser_instance_) {
            reset_plugins_(parser_instance_);
        }
    }

    void Shutdown() {
        if (parser_instance_ && destroy_parser_) {
            destroy_parser_(parser_instance_);
            parser_instance_ = nullptr;
        }

        if (core_dll_) {
            FreeLibrary(core_dll_);
            core_dll_ = nullptr;
        }
    }

    swtor::plugin_manager* main_parser;

private:
    HMODULE core_dll_ = nullptr;
    void* parser_instance_ = nullptr;
    
    // Function pointers
    CreateParserFunc create_parser_ = nullptr;
    DestroyParserFunc destroy_parser_ = nullptr;
    GetPluginManagerFunc get_plugin_manager_ = nullptr;
    LoadPluginFunc load_plugin_ = nullptr;
    LoadPluginDirectFunc load_plugin_direct_ = nullptr;
    ProcessLineFunc process_line_ = nullptr;
    ResetPluginsFunc reset_plugins_ = nullptr;
};

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

fs::path findFile(
    std::string_view filename,
    std::span<const fs::path> searchDirs)
{
    std::error_code ec;
    fs::path filenamePath{ filename };

    // 1. Search through provided directories
    for (const auto& dir : searchDirs) {
        fs::path candidate = dir / filenamePath;

        if (fs::exists(candidate, ec) && fs::is_regular_file(candidate, ec)) {
            fs::path full = fs::canonical(candidate, ec);
            return ec ? candidate : full; // canonical if possible
        }
    }

    // 2. Fall back to the executable directory
    if (fs::path exeDir = GetExecutableDir(); !exeDir.empty()) {
        fs::path candidate = exeDir / filenamePath;

        if (fs::exists(candidate, ec) && fs::is_regular_file(candidate, ec)) {
            fs::path full = fs::canonical(candidate, ec);
            return ec ? candidate : full;
        }
    }

    // 3. Not found: return empty path
    return {};
}

int main(int argc, char* argv[]) {

    std::string default_file = "combat_sample_dummy_log.txt";
    std::vector<fs::path> dirs = {
        "C:\\Temp\\Logs"
    };
    std::cout << "SWTOR Combat Parser Host Application" << std::endl;
    std::cout << "=====================================" << std::endl;

    // Initialize the parser host
    ParserHost host;
    if (!host.Initialize("swtor_parser_core.dll")) {
        std::cerr << "Failed to initialize parser host" << std::endl;
        return 1;
    }

    // Load external plugins
    if (!host.LoadPluginsFromDirectory("plugins")) {
        std::cerr << "Warning: Failed to load plugins from 'plugins' directory" << std::endl;
    }

    // Example usage
    std::cout << "\nParser is ready. Built-in plugins (like TimingPlugin) are loaded." << std::endl;
    std::cout << "External plugins have been loaded from the plugins directory." << std::endl;
    std::cout << "\nYou can now process combat log lines..." << std::endl;

	DamageTesterPlugin* damage_tester = new DamageTesterPlugin();
	host.LoadPluginDirect(static_cast<void*>(damage_tester));

    // Example: Process a sample combat line
    // host.ProcessCombatLine("[23:45:12.345] [@PlayerName] [Some Ability {1234567890}] [Event {836045448945500}]");
	host.main_parser->print_registered_plugins();


    if (argc >= 2) {
        default_file = argv[1];
    }
    fs::path test_file = findFile(default_file, dirs);
    if (!test_file.empty()) {
        std::cout << "Found default test file: " << test_file.string() << std::endl;
        std::cout << "  Reading lines into memory." << std::endl;
        auto raw_lines = readLogFile(test_file.string());
        std::cout << "  Parsing lines through manager and plugins." << std::endl;
        auto t1 = std::chrono::steady_clock::now();
        for (const auto& line : raw_lines) {
            host.ProcessCombatLine(line);
        }
        auto t2 = std::chrono::steady_clock::now();
        const size_t total = raw_lines.size();
        std::cout << "Statistics: " << std::endl;
        std::chrono::duration<double, std::milli> ms = t2 - t1;
        const double total_ms = ms.count();
        const double avg_ms_per_line = total_ms / static_cast<double>(total);
        const double lines_per_sec = (total_ms > 0.0) ? (1000.0 / avg_ms_per_line) : 0.0;
        std::cout << "  Total lines processed: " << total << std::endl;
        std::cout << "  Parse Time Elapsed: " << total_ms << " ms\n";
        std::cout << "  Parse Line Time Average: " << avg_ms_per_line << " ms/line\n";
        std::cout << "  Parse Time Throughput: " << lines_per_sec << " lines/sec\n";
    }
    
    

    std::cout << "\nPress Enter to exit..." << std::endl;
    std::cin.get();

    return 0;
}
