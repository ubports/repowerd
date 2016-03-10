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
    : handler{[](ProximityState){}},
      state{ProximityState::far}
{
}

void rt::FakeProximitySensor::set_proximity_handler(
    ProximityHandler const& handler)
{
    mock.set_proximity_handler(handler);
    this->handler = handler;
}

void rt::FakeProximitySensor::clear_proximity_handler()
{
    mock.clear_proximity_handler();
    handler = [](ProximityState){};
}

repowerd::ProximityState rt::FakeProximitySensor::proximity_state()
{
    return state;
}

void rt::FakeProximitySensor::emit_proximity_state(ProximityState state)
{
    this->state = state;
    handler(state);
}

void rt::FakeProximitySensor::set_proximity_state(ProximityState state)
{
    this->state = state;
}
