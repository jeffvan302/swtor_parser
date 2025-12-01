#include "plugin_api.h"
#include "parse_plugin.h"
#include <windows.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <iostream>

namespace swtor {

    /// <summary>
    /// Wrapper for external plugins loaded from DLLs
    /// </summary>
    class ExternalPluginWrapper : public parse_plugin {
    public:
        ExternalPluginWrapper(void* plugin_ptr, DestroyPluginFunc destroy_func, HMODULE dll_handle)
            : plugin_instance_(plugin_ptr)
            , destroy_func_(destroy_func)
            , dll_handle_(dll_handle)
            , actual_plugin_(static_cast<parse_plugin*>(plugin_ptr))
        {}

        ~ExternalPluginWrapper() override {
            if (plugin_instance_ && destroy_func_) {
                destroy_func_(plugin_instance_);
            }
            if (dll_handle_) {
                FreeLibrary(dll_handle_);
            }
        }

        std::string name() const override {
            return actual_plugin_->name();
        }

        void set_priority(int p) override {
            actual_plugin_->set_priority(p);
        }

        int get_priority() const override {
            return actual_plugin_->get_priority();
        }

        void enable() override {
            actual_plugin_->enable();
        }

        void disable() override {
            actual_plugin_->disable();
        }

        bool is_enabled() const override {
            return actual_plugin_->is_enabled();
        }

        void set_id(parse_data_holder& parse_data, uint16_t plugin_id) override {
            actual_plugin_->set_id(parse_data, plugin_id);
        }

        uint16_t get_id() const override {
            return actual_plugin_->get_id();
        }

        void ingest(parse_data_holder& parse_data, CombatLine& line) override {
            actual_plugin_->ingest(parse_data, line);
        }

        void reset() override {
            actual_plugin_->reset();
        }

    private:
        void* plugin_instance_;
        DestroyPluginFunc destroy_func_;
        HMODULE dll_handle_;
        parse_plugin* actual_plugin_;
    };

    // Static map to track loaded DLLs and prevent duplicate loading
    static std::unordered_map<std::string, HMODULE> loaded_dlls_;

    std::shared_ptr<parse_plugin> PluginLoader::LoadPlugin(const char* path) {
        if (!path) {
            std::cerr << "PluginLoader: Invalid plugin path (nullptr)" << std::endl;
            return nullptr;
        }

        std::string path_str(path);

        // Check if already loaded
        auto it = loaded_dlls_.find(path_str);
        if (it != loaded_dlls_.end()) {
            std::cerr << "PluginLoader: Plugin already loaded: " << path << std::endl;
            return nullptr;
        }

        // Load the DLL
        HMODULE dll_handle = LoadLibraryA(path);
        if (!dll_handle) {
            DWORD error = GetLastError();
            std::cerr << "PluginLoader: Failed to load DLL: " << path 
                      << " (Error code: " << error << ")" << std::endl;
            return nullptr;
        }

        // Get the required functions
        auto create_func = reinterpret_cast<CreatePluginFunc>(
            GetProcAddress(dll_handle, "CreatePlugin"));
        auto destroy_func = reinterpret_cast<DestroyPluginFunc>(
            GetProcAddress(dll_handle, "DestroyPlugin"));
        auto info_func = reinterpret_cast<GetPluginInfoFunc>(
            GetProcAddress(dll_handle, "GetPluginInfo"));

        if (!create_func || !destroy_func || !info_func) {
            std::cerr << "PluginLoader: DLL missing required exports: " << path << std::endl;
            FreeLibrary(dll_handle);
            return nullptr;
        }

        // Verify API version
        PluginInfo info = info_func();
        if (info.api_version != PLUGIN_API_VERSION) {
            std::cerr << "PluginLoader: Plugin API version mismatch. Expected: " 
                      << PLUGIN_API_VERSION << ", Got: " << info.api_version 
                      << " (" << path << ")" << std::endl;
            FreeLibrary(dll_handle);
            return nullptr;
        }

        // Create the plugin instance
        void* plugin_ptr = create_func();
        if (!plugin_ptr) {
            std::cerr << "PluginLoader: Failed to create plugin instance: " << path << std::endl;
            FreeLibrary(dll_handle);
            return nullptr;
        }

        // Store the DLL handle
        loaded_dlls_[path_str] = dll_handle;

        std::cout << "PluginLoader: Successfully loaded plugin: " << info.name 
                  << " v" << info.version << " by " << info.author << std::endl;

        // Create wrapper
        auto wrapper = std::make_shared<ExternalPluginWrapper>(
            plugin_ptr, destroy_func, dll_handle);

        return wrapper;
    }

    void PluginLoader::UnloadPlugin(std::shared_ptr<parse_plugin> plugin) {
        // The wrapper's destructor will handle cleanup
        plugin.reset();
    }

    PluginInfo PluginLoader::GetPluginInfo(const char* path) {
        PluginInfo empty_info = { "Unknown", "0.0.0", "Unknown", "Failed to load", 0 };

        if (!path) {
            return empty_info;
        }

        HMODULE dll_handle = LoadLibraryA(path);
        if (!dll_handle) {
            return empty_info;
        }

        auto info_func = reinterpret_cast<GetPluginInfoFunc>(
            GetProcAddress(dll_handle, "GetPluginInfo"));

        PluginInfo info = empty_info;
        if (info_func) {
            info = info_func();
        }

        FreeLibrary(dll_handle);
        return info;
    }

    ExternalPluginBase::ExternalPluginBase() = default;
    ExternalPluginBase::~ExternalPluginBase() = default;
    
    // ExternalPluginBase helper implementations
    bool ExternalPluginBase::IsEventType(const CombatLine& line, uint64_t event_type) const {
        return line.event.type_id == event_type;
    }

    bool ExternalPluginBase::IsPlayer(const parse_data_holder& parse_data, uint64_t entity_id) const {
        if (!parse_data.entities) {
            return false;
        }
        // This would need proper implementation based on EntityManager
        // Placeholder for now
        return false;
    }

    int64_t ExternalPluginBase::GetCombatTimeMs(const parse_data_holder& parse_data) const {
        if (!parse_data.ntp_keeper) {
            return 0;
        }
        return parse_data.ntp_keeper->get_LocalTime_in_epoch_ms();
    }

    int64_t ExternalPluginBase::GetTimeSinceLastEventMs(const parse_data_holder& parse_data) const {
        if (!parse_data.ntp_keeper) {
            return 0;
        }
        int64_t current_time = parse_data.ntp_keeper->get_LocalTime_in_epoch_ms();
        int64_t last_time = parse_data.last_line.t.refined_epoch_ms;
        return current_time - last_time;
    }

} // namespace swtor
