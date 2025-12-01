#pragma once

/*
 * SWTOR Combat Parser - Plugin SDK
 * 
 * This header provides everything you need to create external plugins
 * for the SWTOR Combat Parser.
 * 
 * Quick Start:
 * 1. Include this header in your plugin .cpp file
 * 2. Create a class that inherits from swtor::ExternalPluginBase
 * 3. Implement the required virtual methods
 * 4. Use the EXPORT_PLUGIN macro at the end of your file
 * 5. Compile as a DLL linking against swtor_parser_core.lib
 * 
 * Example:
 *     #include "plugin_sdk.h"
 *     
 *     class MyPlugin : public swtor::ExternalPluginBase {
 *     public:
 *         std::string name() const override { return "MyPlugin"; }
 *         
 *         PluginInfo GetInfo() const override {
 *             return {"MyPlugin", "1.0.0", "Me", "My awesome plugin", PLUGIN_API_VERSION};
 *         }
 *         
 *         void ingest(swtor::parse_data_holder& parse_data, swtor::CombatLine& line) override {
 *             // Process combat line here
 *         }
 *         
 *         void reset() override {
 *             // Reset plugin state
 *         }
 *     };
 *     
 *     EXPORT_PLUGIN(MyPlugin)
 */

// Include all necessary headers from the core library
#include "../core/plugin_api.h"
#include "../core/parse_plugin.h"
#include "../core/swtor_parser.h"
#include "../core/combat_state.h"
#include "../core/ntp_timekeeper_swtor.h"
#include "../core/time_cruncher_swtor.h"

namespace swtor {

/*
 * Common Event Types (from swtor_parser.h)
 * Use these constants with IsEventType() helper method
 */

// Main event kinds

/*
 * Plugin Development Guidelines:
 * 
 * 1. Thread Safety: Your plugin's ingest() method may be called from multiple threads.
 *    Use appropriate synchronization if you maintain shared state.
 * 
 * 2. Performance: The ingest() method is called for every combat log line.
 *    Keep processing fast to avoid bottlenecks.
 * 
 * 3. Memory Management: The plugin instance is owned by the parser.
 *    Clean up your resources in the destructor and reset() method.
 * 
 * 4. Error Handling: Catch exceptions in your code. Uncaught exceptions
 *    may crash the host application.
 * 
 * 5. API Version: Always set api_version to PLUGIN_API_VERSION in GetInfo().
 *    The parser will refuse to load plugins with mismatched API versions.
 * 
 * 6. Priority: Use set_priority() to control execution order. Lower values
 *    execute first. Default is 0. Negative values are reserved for internal use.
 */

} // namespace swtor

/*
 * Linking Instructions:
 * 
 * When compiling your plugin DLL, you need to:
 * 
 * 1. Add the SDK include directory to your include path
 * 2. Link against swtor_parser_core.lib (generated when building the core DLL)
 * 3. Define your plugin project as a DLL (Dynamic Library)
 * 4. Set C++ standard to C++20 or later
 * 5. Ensure your DLL exports the required functions using EXPORT_PLUGIN macro
 * 
 * Visual Studio Project Settings:
 * - Configuration Type: Dynamic Library (.dll)
 * - C++ Language Standard: C++20 or later
 * - Additional Include Directories: (path to SDK headers)
 * - Additional Library Directories: (path to swtor_parser_core.lib)
 * - Additional Dependencies: swtor_parser_core.lib
 * 
 * The compiled DLL should be placed in the "plugins" directory
 * next to the parser_host.exe executable.
 */
