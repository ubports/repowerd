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

#pragma once

#include <vector>
#include <memory>

#include <android/hardware/lights.h>

namespace repowerd
{
namespace test
{

class FakeLibhardware
{
public:
    FakeLibhardware();
    ~FakeLibhardware();

    bool is_lights_module_open();
    std::vector<light_state_t> backlight_state_history();

    static int static_hw_get_module(char const* id, hw_module_t const** module);

private:
    static FakeLibhardware* fake_libhardware_instance;

    int hw_get_module(char const* id, hw_module_t const** module);
    int set_light(light_state_t const* state);

    std::unique_ptr<hw_module_t> lights_module;
    std::vector<light_state_t> backlight_state_history_;
};

}
}
