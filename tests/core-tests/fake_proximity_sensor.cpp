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

#include "fake_proximity_sensor.h"

namespace rt = repowerd::test;

rt::FakeProximitySensor::FakeProximitySensor()
    : events_enabled{false},
      handler{[](ProximityState){}},
      state{ProximityState::far}
{
}

repowerd::HandlerRegistration rt::FakeProximitySensor::register_proximity_handler(
    ProximityHandler const& handler)
{
    mock.register_proximity_handler(handler);
    this->handler = handler;
    return HandlerRegistration{
        [this]
        {
            mock.unregister_proximity_handler();
            this->handler = [](ProximityState){};
        }};
}

repowerd::ProximityState rt::FakeProximitySensor::proximity_state()
{
    return state;
}

void rt::FakeProximitySensor::enable_proximity_events()
{
    events_enabled = true;
}

void rt::FakeProximitySensor::disable_proximity_events()
{
    events_enabled = false;
}

void rt::FakeProximitySensor::emit_proximity_state(ProximityState state)
{
    this->state = state;
    handler(state);
}

void rt::FakeProximitySensor::emit_proximity_state_if_enabled(ProximityState state)
{
    this->state = state;
    if (events_enabled)
        handler(state);
}

void rt::FakeProximitySensor::set_proximity_state(ProximityState state)
{
    this->state = state;
}

bool rt::FakeProximitySensor::are_proximity_events_enabled()
{
    return events_enabled;
}
