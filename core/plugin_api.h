#pragma once

#include <memory>
#include <cstdint>
#include "base_classes.h"
#include "parse_manager.h"

// Forward declarations
//namespace swtor {
//    struct parse_plugin;
//    struct parse_data_holder;
//    struct CombatLine;
//}

// Plugin API Export/Import Macros
#ifdef SWTOR_PARSER_EXPORTS
    #define SWTOR_API __declspec(dllexport)
#else
    #define SWTOR_API __declspec(dllimport)
#endif

// C-style plugin interface for ABI stability across compilers
extern "C" {
    /// <summary>
    /// Plugin information structure returned by plugins
    /// </summary>
    struct PluginInfo {
        const char* name;
        const char* version;
        const char* author;
        const char* description;
        int api_version; // Must match PLUGIN_API_VERSION
    };

    /// <summary>
    /// Current plugin API version - plugins must match this
    /// </summary>
    constexpr int PLUGIN_API_VERSION = 1;

    /// <summary>
    /// Function signature for plugin creation
    /// Creates and returns a new plugin instance
    /// </summary>
    typedef void* (*CreatePluginFunc)();

    /// <summary>
    /// Function signature for plugin destruction
    /// Properly destroys a plugin instance
    /// </summary>
    typedef void (*DestroyPluginFunc)(void* plugin);

    /// <summary>
    /// Function signature for getting plugin information
    /// Returns metadata about the plugin
    /// </summary>
    typedef PluginInfo (*GetPluginInfoFunc)();
}

namespace swtor {
    // Forward declarations
    //struct parse_plugin;
    //struct parse_data_holder;

    /// <summary>
    /// Plugin loader interface for loading external plugin DLLs
    /// </summary>
    class SWTOR_API PluginLoader {
    public:
        /// <summary>
        /// Load a plugin from a DLL file
        /// </summary>
        /// <param name="path">Path to the plugin DLL</param>
        /// <returns>Shared pointer to the loaded plugin, or nullptr on failure</returns>
        static std::shared_ptr<parse_plugin> LoadPlugin(const char* path);

        /// <summary>
        /// Unload a plugin and free its resources
        /// </summary>
        /// <param name="plugin">Plugin to unload</param>
        static void UnloadPlugin(std::shared_ptr<parse_plugin> plugin);

        /// <summary>
        /// Get information about a plugin DLL without loading it
        /// </summary>
        /// <param name="path">Path to the plugin DLL</param>
        /// <returns>Plugin information structure</returns>
        static PluginInfo GetPluginInfo(const char* path);
    };

    /// <summary>
    /// Base class for external plugins to inherit from
    /// Provides helper functions and simplified interface
    /// </summary>
    class SWTOR_API ExternalPluginBase : public parse_plugin {
    public:
        ExternalPluginBase();
        virtual ~ExternalPluginBase();

        /// <summary>
        /// Get plugin metadata - override this in your plugin
        /// </summary>
        virtual PluginInfo GetInfo() const = 0;

        /// <summary>
        /// Helper: Check if current event is a specific type
        /// </summary>
        bool IsEventType(const CombatLine& line, uint64_t event_type) const;

        /// <summary>
        /// Helper: Check if entity is the player
        /// </summary>
        bool IsPlayer(const parse_data_holder& parse_data, uint64_t entity_id) const;

        /// <summary>
        /// Helper: Get current combat time in milliseconds
        /// </summary>
        int64_t GetCombatTimeMs(const parse_data_holder& parse_data) const;

        /// <summary>
        /// Helper: Get time since last event in milliseconds
        /// </summary>
        int64_t GetTimeSinceLastEventMs(const parse_data_holder& parse_data) const;
    };
} // namespace swtor

// Macro to simplify plugin export in external DLLs
#define EXPORT_PLUGIN(PluginClass) \
    extern "C" { \
        __declspec(dllexport) void* CreatePlugin() { \
            return new PluginClass(); \
        } \
        __declspec(dllexport) void DestroyPlugin(void* plugin) { \
            delete static_cast<PluginClass*>(plugin); \
        } \
        __declspec(dllexport) PluginInfo GetPluginInfo() { \
            static PluginClass temp_instance; \
            return temp_instance.GetInfo(); \
        } \
    }
