/*
 * Copyright © 2020 UBports foundation
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
 * Authored by: Marius Gripsgard <marius@ubports.com>
 */

#include "sensorfw_light_sensor.h"
#include "event_loop_handler_registration.h"

#include "socketreader.h"

#include <stdexcept>

namespace
{
auto const null_handler = [](double){};
}

repowerd::SensorfwLightSensor::SensorfwLightSensor(
    std::shared_ptr<Log> const& log,
    std::string const& dbus_bus_address)
    : Sensorfw(log, dbus_bus_address, "Light", PluginType::LIGHT),
      handler{null_handler}
{
}

repowerd::HandlerRegistration repowerd::SensorfwLightSensor::register_light_handler(
    LightHandler const& handler)
{
    return EventLoopHandlerRegistration{
        dbus_event_loop,
        [this, &handler]{ this->handler = handler; },
        [this]{ this->handler = null_handler; }};
}

void repowerd::SensorfwLightSensor::enable_light_events()
{
    dbus_event_loop.enqueue(
        [this]
        {
            start();
        }).get();
}

void repowerd::SensorfwLightSensor::disable_light_events()
{
    dbus_event_loop.enqueue(
        [this]
        {
            stop();
        }).get();
}

void repowerd::SensorfwLightSensor::data_recived_impl()
{
    QVector<TimedUnsigned> values;
    if(!m_socket->read<TimedUnsigned>(values))
        return;

    handler(values[0].value_);
}
