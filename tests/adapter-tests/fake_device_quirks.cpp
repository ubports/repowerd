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

#include "fake_device_quirks.h"

namespace rt = repowerd::test;

rt::FakeDeviceQuirks::FakeDeviceQuirks()
    : synthetic_initial_event_type_{DeviceQuirks::ProximityEventType::far},
      normal_before_display_on_autobrightness_{false}
{
}

std::chrono::milliseconds
rt::FakeDeviceQuirks::synthetic_initial_proximity_event_delay() const
{
    // Set a high delay to account for valgrind slowness
    return std::chrono::milliseconds{1000};
}

repowerd::DeviceQuirks::ProximityEventType
rt::FakeDeviceQuirks::synthetic_initial_proximity_event_type() const
{
    return synthetic_initial_event_type_;
}

bool rt::FakeDeviceQuirks::normal_before_display_on_autobrightness() const
{
    return normal_before_display_on_autobrightness_;
}

void rt::FakeDeviceQuirks::set_synthetic_initial_event_type_near()
{
    synthetic_initial_event_type_ = repowerd::DeviceQuirks::ProximityEventType::near;
}

void rt::FakeDeviceQuirks::set_normal_before_display_on_autobrightness(bool value)
{
    normal_before_display_on_autobrightness_ = value;
}
