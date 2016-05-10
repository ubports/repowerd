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

#include "autobrightness_algorithm.h"
#include "backlight_brightness_control.h"
#include "backlight.h"
#include "brightness_params.h"
#include "light_sensor.h"

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
    std::shared_ptr<LightSensor> const& light_sensor,
    std::shared_ptr<AutobrightnessAlgorithm> const& autobrightness_algorithm,
    DeviceConfig const& device_config)
    : backlight{backlight},
      light_sensor{light_sensor},
      autobrightness_algorithm{autobrightness_algorithm},
      ab_supported{autobrightness_algorithm->init(event_loop)},
      dim_brightness{dim_brightness_percent(device_config)},
      normal_brightness{normal_brightness_percent(device_config)},
      user_normal_brightness{normal_brightness},
      ab_active{false}
{
    if (ab_supported)
    {
        ab_handler_registration = autobrightness_algorithm->register_autobrightness_handler(
            [this] (double brightness)
            {
                if (ab_active)
                {
                    normal_brightness = brightness;

                    if (normal_brightness_active)
                        transition_to_brightness_value(normal_brightness, TransitionSpeed::slow);
                }
            });

        light_handler_registration = light_sensor->register_light_handler(
            [this] (double light)
            {
                this->autobrightness_algorithm->new_light_value(light);
            });
    }
}

void repowerd::BacklightBrightnessControl::disable_autobrightness()
{
    if (!ab_supported) return;

    event_loop.enqueue(
        [this]
        {
            if (ab_active)
            {
                light_sensor->disable_light_events();
                normal_brightness = user_normal_brightness;
                ab_active = false;
                if (normal_brightness_active)
                    transition_to_brightness_value(normal_brightness, TransitionSpeed::slow);
            }
        });
}

void repowerd::BacklightBrightnessControl::enable_autobrightness()
{
    if (!ab_supported) return;

    event_loop.enqueue(
        [this]
        { 
            if (!ab_active)
            {
                autobrightness_algorithm->reset();
                light_sensor->enable_light_events();
                ab_active = true;
            }
        });
}

void repowerd::BacklightBrightnessControl::set_dim_brightness()
{
    event_loop.enqueue(
        [this]
        { 
            transition_to_brightness_value(dim_brightness, TransitionSpeed::normal);
            normal_brightness_active = false;
        });
}

void repowerd::BacklightBrightnessControl::set_normal_brightness()
{
    event_loop.enqueue(
        [this]
        { 
            transition_to_brightness_value(normal_brightness, TransitionSpeed::normal);
            normal_brightness_active = true;
        });
}

void repowerd::BacklightBrightnessControl::set_normal_brightness_value(float v)
{
    event_loop.enqueue(
        [this,v]
        { 
            user_normal_brightness = v;
            if (normal_brightness_active && !ab_active)
            {
                normal_brightness = user_normal_brightness;
                transition_to_brightness_value(normal_brightness, TransitionSpeed::normal);
            }
        });
}

void repowerd::BacklightBrightnessControl::set_off_brightness()
{
    event_loop.enqueue(
        [this]
        { 
            transition_to_brightness_value(0, TransitionSpeed::normal);
            normal_brightness_active = false;
        });
}

void repowerd::BacklightBrightnessControl::sync()
{
    event_loop.enqueue([]{}).get();
}

void repowerd::BacklightBrightnessControl::transition_to_brightness_value(
    float brightness, TransitionSpeed transition_speed)
{
    auto const starting_brightness = get_brightness_value();
    auto current_brightness = starting_brightness;
    auto const step = 0.01;
    auto const num_steps = fabs(current_brightness - brightness) / step;
    auto const step_time = (transition_speed == TransitionSpeed::slow ||
                            starting_brightness == 0.0f
                            || brightness == 0.0f) ?
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
