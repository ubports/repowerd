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

#include "ubuntu_light_sensor.h"
#include "event_loop_handler_registration.h"

#include <stdexcept>

namespace
{
auto const null_handler = [](double){};
}

repowerd::UbuntuLightSensor::UbuntuLightSensor()
    : sensor{ua_sensors_light_new()},
      event_loop{"Light"},
      handler{null_handler},
      enabled{false}
{
    if (!sensor)
        throw std::runtime_error("Failed to allocate light sensor");

    ua_sensors_light_set_reading_cb(sensor, static_sensor_reading_callback, this);
}

repowerd::HandlerRegistration repowerd::UbuntuLightSensor::register_light_handler(
    LightHandler const& handler)
{
    return EventLoopHandlerRegistration{
        event_loop,
        [this, &handler]{ this->handler = handler; },
        [this]{ this->handler = null_handler; }};
}

void repowerd::UbuntuLightSensor::enable_light_events()
{
    event_loop.enqueue(
        [this]
        {
            if (!enabled)
            {
                ua_sensors_light_enable(sensor);
                enabled = true;
            }
        }).get();
}

void repowerd::UbuntuLightSensor::disable_light_events()
{
    event_loop.enqueue(
        [this]
        {
            if (enabled)
            {
                ua_sensors_light_disable(sensor);
                enabled = false;
            }
        }).get();
}

void repowerd::UbuntuLightSensor::static_sensor_reading_callback(
    UASLightEvent* event, void* context)
{
    auto const uls = static_cast<UbuntuLightSensor*>(context);
    float light_value{0.0f};
    uas_light_event_get_light(event, &light_value);
    uls->event_loop.enqueue([uls, light_value] { uls->handle_light_event(light_value); });
}

void repowerd::UbuntuLightSensor::handle_light_event(double light)
{
    handler(light);
}
