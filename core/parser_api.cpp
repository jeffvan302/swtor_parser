#include "parse_manager.h"
#include "plugin_api.h"
#include "timing_plugin.h"
#include <memory>

// C API for host application to interact with the core library
extern "C" {
    
    /// <summary>
    /// Create a new parser instance with plugin manager
    /// </summary>
    __declspec(dllexport) void* CreateParser() {
        try {
            auto* manager = new swtor::plugin_manager();
            
             //Register built-in plugins
   //         auto timing_plugin = std::make_shared<swtor::TimingPlugin>();
			//timing_plugin->set_priority(0); // High priority    
   //         manager->register_plugin(timing_plugin);
   //         
   //         auto test_plugin = std::make_shared<swtor::TestPlugin>();
   //         test_plugin->set_priority(10);
   //         manager->register_plugin(test_plugin);
            
            return manager;
        } catch (const std::exception& e) {
            std::cerr << "Error creating parser: " << e.what() << std::endl;
            return nullptr;
        }
    }

    /// <summary>
    /// Destroy a parser instance
    /// </summary>
    __declspec(dllexport) void DestroyParser(void* parser) {
        if (parser) {
            delete static_cast<swtor::plugin_manager*>(parser);
        }
    }

    /// <summary>
    /// Get the plugin manager from a parser instance
    /// </summary>
    __declspec(dllexport) void* GetPluginManager(void* parser) {
        return parser; // Parser IS the plugin manager
    }

    /// <summary>
    /// Load an external plugin from a DLL
    /// </summary>
    __declspec(dllexport) void LoadExternalPlugin(void* parser, const char* plugin_path) {
        if (!parser || !plugin_path) {
            return;
        }

        auto* manager = static_cast<swtor::plugin_manager*>(parser);
        auto plugin = swtor::PluginLoader::LoadPlugin(plugin_path);
        
        if (plugin) {
            plugin->set_priority(100);
            manager->register_plugin(plugin);
        }
    }


    __declspec(dllexport) void LoadExternalPlugin_Direct(void* parser, void* plugin_ptr) {
        if (!parser || !plugin_ptr) {
            return;
        }

        auto* manager = static_cast<swtor::plugin_manager*>(parser);
        auto plugin = static_cast<swtor::parse_plugin*>(plugin_ptr);

        if (plugin) {
            plugin->set_priority(100);
            manager->register_plugin(plugin);
        }
    }


    /// <summary>
    /// Process a combat line string
    /// </summary>
    __declspec(dllexport) void ProcessCombatLine(void* parser, const char* line) {
        if (!parser || !line) {
            return;
        }

        try {
            auto* manager = static_cast<swtor::plugin_manager*>(parser);
            manager->process_line(std::string(line));
        } catch (const std::exception& e) {
            std::cerr << "Error processing line: " << e.what() << std::endl;
        }
    }

    /// <summary>
    /// Reset all plugins in the parser
    /// </summary>
    __declspec(dllexport) void ResetAllPlugins(void* parser) {
        if (!parser) {
            return;
        }

        try {
            auto* manager = static_cast<swtor::plugin_manager*>(parser);
            manager->reset_plugins();
        } catch (const std::exception& e) {
            std::cerr << "Error resetting plugins: " << e.what() << std::endl;
        }
    }

    /// <summary>
    /// Get plugin by name
    /// </summary>
    __declspec(dllexport) void* GetPluginByName(void* parser, const char* name) {
        if (!parser || !name) {
            return nullptr;
        }

        auto* manager = static_cast<swtor::plugin_manager*>(parser);
        auto plugin = manager->get_plugin_by_name(name);
        return plugin.get();
    }

    /// <summary>
    /// Get plugin by ID
    /// </summary>
    __declspec(dllexport) void* GetPluginById(void* parser, uint16_t id) {
        if (!parser) {
            return nullptr;
        }

        auto* manager = static_cast<swtor::plugin_manager*>(parser);
        auto plugin = manager->get_plugin_by_id(id);
        return plugin.get();
    }

    /// <summary>
    /// Check if parser is in combat
    /// </summary>
    __declspec(dllexport) bool IsInCombat(void* parser) {
        if (!parser) {
            return false;
        }

        auto* manager = static_cast<swtor::plugin_manager*>(parser);
        return manager->is_in_combat();
    }

} // extern "C"
