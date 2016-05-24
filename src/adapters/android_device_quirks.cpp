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

#include "android_device_quirks.h"

#include <hybris/properties/properties.h>

using namespace std::chrono_literals;

namespace
{

std::string determine_device_name()
{
    char name[PROP_VALUE_MAX] = "";
    property_get("ro.product.device", name, "");
    return name;
}

std::chrono::milliseconds synthetic_initial_proximity_event_delay_for(std::string)
{
    return 500ms;
}

repowerd::AndroidDeviceQuirks::ProximityEventType
synthetic_initial_proximity_event_type_for(std::string device_name)
{
    // In general we assume a "near" state if we don't get an initial event.
    // However, arale does not emit an initial event when in the "far" state
    // in particular, so we assume a "far" state for arale.
    if (device_name == "arale")
        return repowerd::AndroidDeviceQuirks::ProximityEventType::far;
    else
        return repowerd::AndroidDeviceQuirks::ProximityEventType::near;
}

}

repowerd::AndroidDeviceQuirks::AndroidDeviceQuirks()
    : device_name_{determine_device_name()},
      synthetic_initial_proximity_event_delay_{
          synthetic_initial_proximity_event_delay_for(device_name_)},
      synthetic_initial_proximity_event_type_{
          synthetic_initial_proximity_event_type_for(device_name_)}
{
}

std::chrono::milliseconds
repowerd::AndroidDeviceQuirks::synthetic_initial_proximity_event_delay() const
{
    return synthetic_initial_proximity_event_delay_;
}

repowerd::AndroidDeviceQuirks::ProximityEventType
repowerd::AndroidDeviceQuirks::synthetic_initial_proximity_event_type() const
{
    return synthetic_initial_proximity_event_type_;
}
