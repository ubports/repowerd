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

#include "backlight_brightness_control.h"
#include "backlight.h"
#include "brightness_params.h"

#include <cmath>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace
{

float normal_brightness_percent(repowerd::DeviceConfig const& device_config)
{
    auto brightness_params = repowerd::BrightnessParams::from_device_config(device_config);
    return static_cast<float>(brightness_params.default_value) / brightness_params.max_value;
}

float dim_brightness_percent(repowerd::DeviceConfig const& device_config)
{
    auto const brightness_params = repowerd::BrightnessParams::from_device_config(device_config);
    return static_cast<float>(brightness_params.dim_value) / brightness_params.max_value;
}

}

repowerd::BacklightBrightnessControl::BacklightBrightnessControl(
    std::shared_ptr<Backlight> const& backlight,
    DeviceConfig const& device_config)
    : backlight{backlight},
      dim_brightness{dim_brightness_percent(device_config)},
      normal_brightness{normal_brightness_percent(device_config)}
{
}

void repowerd::BacklightBrightnessControl::disable_autobrightness()
{
}

void repowerd::BacklightBrightnessControl::enable_autobrightness()
{
}

void repowerd::BacklightBrightnessControl::set_dim_brightness()
{
    transition_to_brightness_value(dim_brightness);
    normal_brightness_active = false;
}

void repowerd::BacklightBrightnessControl::set_normal_brightness()
{
    transition_to_brightness_value(normal_brightness);
    normal_brightness_active = true;
}

void repowerd::BacklightBrightnessControl::set_normal_brightness_value(float v)
{
    normal_brightness = v;
    if (normal_brightness_active)
        set_normal_brightness();
}

void repowerd::BacklightBrightnessControl::set_off_brightness()
{
    transition_to_brightness_value(0);
    normal_brightness_active = false;
}

void repowerd::BacklightBrightnessControl::transition_to_brightness_value(float brightness)
{
    auto const starting_brightness = get_brightness_value();
    auto current_brightness = starting_brightness;
    auto const step = 0.01;
    auto const num_steps = fabs(current_brightness - brightness) / step;
    auto const step_time = (starting_brightness == 0.0f || brightness == 0.0f) ?
                           100000us / num_steps : 1000us;

    if (current_brightness < brightness)
    {
        while (current_brightness < brightness)
        {
            current_brightness += step;
            if (current_brightness > brightness) current_brightness = brightness;
            set_brightness_value(current_brightness);
            std::this_thread::sleep_for(step_time);
        }
    }
    else if (current_brightness > brightness)
    {
        while (current_brightness > brightness)
        {
            current_brightness -= step;
            if (current_brightness < brightness) current_brightness = brightness;
            set_brightness_value(current_brightness);
            std::this_thread::sleep_for(step_time);
        }
    }
}

void repowerd::BacklightBrightnessControl::set_brightness_value(float brightness)
{
    backlight->set_brightness(brightness);
}

float repowerd::BacklightBrightnessControl::get_brightness_value()
{
    return backlight->get_brightness();
}
