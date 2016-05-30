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

#include "android_backlight.h"

#include <android/hardware/lights.h>

#include <stdexcept>
#include <cstring>

repowerd::AndroidBacklight::AndroidBacklight()
    : brightness{0.0}
{
    hw_module_t const* hwmod;

    auto error = hw_get_module(LIGHTS_HARDWARE_MODULE_ID, &hwmod);
    if (error || hwmod == nullptr)
        throw std::runtime_error("Failed to open Android lights module");

    error = hwmod->methods->open(hwmod, LIGHT_ID_BACKLIGHT,
                                 reinterpret_cast<hw_device_t**>(&light_dev));
    if (error || light_dev == nullptr)
        throw std::runtime_error("Failed to open Android backlight device");
}

repowerd::AndroidBacklight::~AndroidBacklight()
{
    light_dev->common.close(reinterpret_cast<hw_device_t*>(light_dev));
}

void repowerd::AndroidBacklight::set_brightness(double value)
{
    int const value_abs = value * 255;

    light_state_t state;
    memset(&state, 0, sizeof(light_state_t));
    state.flashMode = LIGHT_FLASH_NONE;
    state.brightnessMode = BRIGHTNESS_MODE_USER;
    state.color = ((0xffU << 24) | (value_abs << 16) | (value_abs << 8) | value_abs);

    auto const err = light_dev->set_light(light_dev, &state);

    if (!err) brightness = value;
}

double repowerd::AndroidBacklight::get_brightness()
{
    return brightness;
}
