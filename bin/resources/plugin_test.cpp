#include "rocket/asset.hpp"
#include "rocket/plugin/api/api.hpp"
#include "rocket/plugin/api/define_these.hpp"
#include <iostream>

extern "C" {
    rapi::api_t *api = nullptr;
    plugin_capabilities_t *cap = new plugin_capabilities_t;
    plugin_capabilities_t *on_load(rapi::api_t *api) {
        cap->is_lazy_loadable = false;
        cap->needs_expo_renderer2d = true;
        cap->needs_frame_events = true;

        std::stringstream of;
        of << "Version: " << api->api_version << ", Iteration: " << api->api_iteration;
        api->log(of.str(), "plugin_test", "on_load", "warn");

        ::api = api;
        return cap;
    }

    void my_test() {
        api->log("This would probably be a publicly-documented function!", "plugin_test", "my_test", "warn");
    }

    void on_framestart() {
        if (api->get_renderer2d /* the funcptr */ == nullptr) {
            return;
        }

        auto *ren = api->get_renderer2d();

        rocket::text_t text = { "hello from plugin!", 24, rocket::rgb_color::black() };
        ren->draw_text(text, { 240, 240 });
    }

    void on_frameend() {
    }

    void on_unload() {
    }
}
