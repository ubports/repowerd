/*
 * Copyright © 2017 Canonical Ltd.
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

#include "repowerd_dbus_client.h"

#include <glib.h>

namespace rt = repowerd::test;

namespace
{
char const* const repowerd_interface = "com.canonical.repowerd";
}

rt::RepowerdDBusClient::RepowerdDBusClient(std::string const& address)
    : rt::DBusClient{
        address,
        "com.canonical.repowerd",
        "/com/canonical/repowerd"}
{
}

rt::DBusAsyncReplyString rt::RepowerdDBusClient::request_introspection()
{
    return invoke_with_reply<rt::DBusAsyncReplyString>(
        "org.freedesktop.DBus.Introspectable", "Introspect",
        nullptr);
}

rt::DBusAsyncReplyVoid rt::RepowerdDBusClient::request_set_inactivity_behavior(
    std::string const& power_action,
    std::string const& power_supply,
    int32_t timeout_sec)
{
    return invoke_with_reply<rt::DBusAsyncReplyVoid>(
        repowerd_interface, "SetInactivityBehavior",
        g_variant_new("(ssi)", power_action.c_str(), power_supply.c_str(), timeout_sec));
}

rt::DBusAsyncReplyVoid rt::RepowerdDBusClient::request_set_lid_behavior(
    std::string const& power_action,
    std::string const& power_supply)
{
    return invoke_with_reply<rt::DBusAsyncReplyVoid>(
        repowerd_interface, "SetLidBehavior",
        g_variant_new("(ss)", power_action.c_str(), power_supply.c_str()));
}

rt::DBusAsyncReplyVoid rt::RepowerdDBusClient::request_set_critical_power_behavior(
    std::string const& power_action)
{
    return invoke_with_reply<rt::DBusAsyncReplyVoid>(
        repowerd_interface, "SetCriticalPowerBehavior",
        g_variant_new("(s)", power_action.c_str()));
}
