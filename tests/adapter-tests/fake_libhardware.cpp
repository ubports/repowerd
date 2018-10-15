/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "fake_libhardware.h"
#include <functional>
#include <stdexcept>

namespace rt = repowerd::test;

namespace
{

struct HwModule : hw_module_t
{
    HwModule(std::function<int(const char* id, hw_device_t**)> const& open_func)
        : open_func{open_func}
    {
        fake_methods.open = static_open;
        methods = &fake_methods;
    }

    static int static_open(hw_module_t const* module, const char* id, hw_device_t** device)
    {
        auto fake_module = reinterpret_cast<HwModule const*>(module);
        return fake_module->open_func(id, device);
    }

    std::function<int(const char*, hw_device_t**)> const open_func;
    hw_module_methods_t fake_methods;
};

struct LightHwDevice : light_device_t
{
    LightHwDevice(
        std::function<int()> const& close_func,
        std::function<int(light_state_t const*)> const& set_light_func)
        : close_func{close_func},
          set_light_func{set_light_func}
    {
        common.close = static_close;
        set_light = static_set_light;
    }

    static int static_close(hw_device_t* dev)
    {
        auto light_device = reinterpret_cast<LightHwDevice*>(dev);
        return light_device->close_func();
    }

    static int static_set_light(light_device_t* dev, light_state_t const* state)
    {
        auto light_device = reinterpret_cast<LightHwDevice*>(dev);
        return light_device->set_light_func(state);
    }

    std::function<int()> const close_func;
    std::function<int(light_state_t const*)> const set_light_func;
};

struct LightsHwModule : HwModule
{
    LightsHwModule(std::function<int(light_state_t const*)> const& set_light_func)
        : HwModule(
            [this] (const char* id, hw_device_t** device) -> int
            {
                return lights_open(id, device);
            }),
          fake_light_device{[this]{ return lights_close(); }, set_light_func},
          is_open{false}
    {
    }

    int lights_open(char const* id, hw_device_t** device)
    {
        if (std::string{id} == LIGHT_ID_BACKLIGHT)
        {
            *device = &fake_light_device.common;
            is_open = true;
            return 0;
        }
        else
        {
            *device = nullptr;
            return -1;
        }
    }

    int lights_close()
    {
        is_open = false;
        return 0;
    }

    LightHwDevice fake_light_device;
    bool is_open;
};

}

int hw_get_module(char const* id, hw_module_t const** module)
{
    return rt::FakeLibhardware::static_hw_get_module(id, module);
}

rt::FakeLibhardware* rt::FakeLibhardware::FakeLibhardware::fake_libhardware_instance = nullptr;

rt::FakeLibhardware::FakeLibhardware()
    : lights_module{
        std::make_unique<LightsHwModule>(
            [this] (light_state_t const* state) -> int
            {
                return set_light(state);
            })}
{
    fake_libhardware_instance = this;
}

rt::FakeLibhardware::~FakeLibhardware()
{
    fake_libhardware_instance = nullptr;
}

bool rt::FakeLibhardware::is_lights_module_open()
{
    return reinterpret_cast<LightsHwModule*>(lights_module.get())->is_open;
}

std::vector<light_state_t> rt::FakeLibhardware::backlight_state_history()
{
    return backlight_state_history_;
}

int rt::FakeLibhardware::static_hw_get_module(char const* id, hw_module_t const** module)
{
    if (!fake_libhardware_instance)
        throw std::runtime_error("Fake libhardware instance not initialized");
    return fake_libhardware_instance->hw_get_module(id, module);
}

int rt::FakeLibhardware::hw_get_module(char const* id, hw_module_t const** module)
{
    if (std::string{id} == LIGHTS_HARDWARE_MODULE_ID)
    {
        *module = lights_module.get();
        return 0;
    }
    else
    {
        *module = nullptr;
        return -1;
    }
}

int rt::FakeLibhardware::set_light(light_state_t const* state)
{
    backlight_state_history_.push_back(*state);
    return 0;
}
