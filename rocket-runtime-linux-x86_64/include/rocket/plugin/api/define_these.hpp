#ifndef ROCKETGE_SUBPLUGIN__DEFINE_THESE_HPP
#define ROCKETGE_SUBPLUGIN__DEFINE_THESE_HPP

#include "rocket/plugin/api/api.hpp"

extern "C" {
    plugin_capabilities_t *on_load(rapi::api_t *api);
    /// @brief Behaviour to execute when LazyMode is began
    void lazymode_begin_lazy();
    /// @brief Behaviour to execute when LazyMode is ended
    void lazymode_end_lazy();
    /// @brief Called on Frame Start
    void on_framestart();
    /// @brief Called on Frame End
    void on_frameend();
    void on_unload();
}

#endif // ROCKETGE_SUBPLUGIN__DEFINE_THESE_HPP
