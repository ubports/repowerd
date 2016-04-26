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

#include "unity_power_button.h"
#include "event_loop_handler_registration.h"

namespace
{
auto const null_handler = [](repowerd::PowerButtonState){};
char const* const dbus_power_button_name = "com.canonical.Unity.PowerButton";
char const* const dbus_power_button_path = "/com/canonical/Unity/PowerButton";
char const* const dbus_power_button_interface = "com.canonical.Unity.PowerButton";
}

repowerd::UnityPowerButton::UnityPowerButton(
    std::string const& dbus_bus_address)
    : dbus_connection{dbus_bus_address},
      power_button_handler{null_handler}
{
}

void repowerd::UnityPowerButton::start_processing()
{
    dbus_signal_handler_registration = dbus_event_loop.register_signal_handler(
        dbus_connection,
        dbus_power_button_name,
        dbus_power_button_interface,
        nullptr,
        dbus_power_button_path,
        [this] (
            GDBusConnection* connection,
            gchar const* sender,
            gchar const* object_path,
            gchar const* interface_name,
            gchar const* signal_name,
            GVariant* parameters)
        {
            handle_dbus_signal(
                connection, sender, object_path, interface_name,
                signal_name, parameters);
        });
}

repowerd::HandlerRegistration repowerd::UnityPowerButton::register_power_button_handler(
    PowerButtonHandler const& handler)
{
    return EventLoopHandlerRegistration{
        dbus_event_loop,
            [this, &handler] { this->power_button_handler = handler; },
            [this] { this->power_button_handler = null_handler; }};
}

void repowerd::UnityPowerButton::notify_long_press()
{
    g_dbus_connection_emit_signal(
        dbus_connection,
        nullptr,
        dbus_power_button_path,
        dbus_power_button_interface,
        "LongPress",
        nullptr,
        nullptr);
}

void repowerd::UnityPowerButton::handle_dbus_signal(
    GDBusConnection* /*connection*/,
    gchar const* /*sender*/,
    gchar const* /*object_path*/,
    gchar const* /*interface_name*/,
    gchar const* signal_name_cstr,
    GVariant* /*parameters*/)
{
    std::string const signal_name{signal_name_cstr ? signal_name_cstr : ""};

    if (signal_name == "Press")
        power_button_handler(repowerd::PowerButtonState::pressed);
    else if (signal_name == "Release")
        power_button_handler(repowerd::PowerButtonState::released);
}
