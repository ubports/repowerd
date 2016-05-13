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

#include "unity_display_power_control.h"

#include <gio/gio.h>

namespace
{
char const* const unity_display_bus_name = "com.canonical.Unity.Display";
char const* const unity_display_object_path = "/com/canonical/Unity/Display";
char const* const unity_display_interface_name = "com.canonical.Unity.Display";
char const* const log_tag = "UnityDisplayPowerControl";
}

repowerd::UnityDisplayPowerControl::UnityDisplayPowerControl(
    std::shared_ptr<Log> const& log,
    std::string const& dbus_bus_address)
    : log{log},
      dbus_connection{dbus_bus_address}
{
}

void repowerd::UnityDisplayPowerControl::turn_on()
{
    log->log(log_tag, "turn_on()");

    g_dbus_connection_call(
        dbus_connection,
        unity_display_bus_name,
        unity_display_object_path,
        unity_display_interface_name,
        "TurnOn",
        nullptr,
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        nullptr,
        nullptr);
}

void repowerd::UnityDisplayPowerControl::turn_off()
{
    log->log(log_tag, "turn_off()");

    g_dbus_connection_call(
        dbus_connection,
        unity_display_bus_name,
        unity_display_object_path,
        unity_display_interface_name,
        "TurnOff",
        nullptr,
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        nullptr,
        nullptr);
}
