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

#include "src/core/log.h"

#include <deviceinfo.h>

using namespace std::chrono_literals;

namespace
{

char const* const log_tag = "AndroidDeviceQuirks";

std::string determine_device_name()
{
    DeviceInfo info;
    return info.name();
}

std::chrono::milliseconds synthetic_initial_proximity_event_delay_for(std::string const& device_name)
{
    // Mako can be a bit slow to report proximity when waking from suspend
    if (device_name == "mako")
        return 700ms;
    else
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

bool normal_before_display_on_autobrightness_for(std::string const& device_name)
{
    auto const quirk_cstr = getenv("REPOWERD_QUIRK_NORMAL_BEFORE_DISPLAY_ON_AUTOBRIGHTNESS");
    std::string const quirk{quirk_cstr ? quirk_cstr : "always"};

    // Mako needs us to manually set the brightness before the first
    // autobrightness setting after turning on the screen. Otherwise, it
    // doesn't update screen brightness to the proper level until the next
    // screen content refresh.
    return (device_name == "mako" || quirk == "always") && quirk != "never";
}

std::string proximity_event_type_to_str(
    repowerd::AndroidDeviceQuirks::ProximityEventType const type)
{
    switch (type)
    {
        case repowerd::AndroidDeviceQuirks::ProximityEventType::near: return "near";
        case repowerd::AndroidDeviceQuirks::ProximityEventType::far: return "far";
        default: return "unknown";
    };

    return "unknown";
}

bool ignore_session_deactivation_for(std::string const& device_name)
{
    // ignore session deactivation for all android devices
    return !device_name.empty();
}

}

repowerd::AndroidDeviceQuirks::AndroidDeviceQuirks(
    repowerd::Log& log)
    : device_name_{determine_device_name()},
      synthetic_initial_proximity_event_delay_{
          synthetic_initial_proximity_event_delay_for(device_name_)},
      synthetic_initial_proximity_event_type_{
          synthetic_initial_proximity_event_type_for(device_name_)},
      normal_before_display_on_autobrightness_{
        normal_before_display_on_autobrightness_for(device_name_)},
      ignore_session_deactivation_{
        ignore_session_deactivation_for(device_name_)}
{
    log.log(log_tag, "DeviceName: %s", device_name_.c_str());
    log.log(log_tag, "Quirk: synthetic_initial_proximit_event_delay=%d",
            static_cast<int>(synthetic_initial_proximity_event_delay_.count()));
    log.log(log_tag, "Quirk: synthetic_initial_proximit_event_type=%s",
            proximity_event_type_to_str(synthetic_initial_proximity_event_type_).c_str());
    log.log(log_tag, "Quirk: normal_before_display_on_autobrightness=%s",
            normal_before_display_on_autobrightness_ ? "true" : "false");
    log.log(log_tag, "Quirk: ignore_session_deactivation=%s",
            ignore_session_deactivation_ ? "true" : "false");
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

bool repowerd::AndroidDeviceQuirks::normal_before_display_on_autobrightness() const
{
    return normal_before_display_on_autobrightness_;
}

bool repowerd::AndroidDeviceQuirks::ignore_session_deactivation() const
{
    return ignore_session_deactivation_;
}
