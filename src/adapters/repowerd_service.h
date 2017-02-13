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

#pragma once

#include "src/core/client_settings.h"

#include "dbus_connection_handle.h"
#include "dbus_event_loop.h"

#include <string>
#include <thread>

#include <sys/types.h>

namespace repowerd
{
class Log;

class RepowerdService : public ClientSettings
{
public:
    RepowerdService(
        std::shared_ptr<Log> const& log,
        std::string const& dbus_bus_address);

    void start_processing() override;

    HandlerRegistration register_set_inactivity_behavior_handler(
        SetInactivityBehaviorHandler const& handler) override;

    HandlerRegistration register_set_lid_behavior_handler(
        SetLidBehaviorHandler const& handler) override;

    HandlerRegistration register_set_critical_power_behavior_handler(
        SetCriticalPowerBehaviorHandler const& handler) override;

private:
    void dbus_method_call(
        GDBusConnection* connection,
        gchar const* sender,
        gchar const* object_path,
        gchar const* interface_name,
        gchar const* method_name,
        GVariant* parameters,
        GDBusMethodInvocation* invocation);

    void dbus_SetInactivityBehavior(
        std::string const& sender,
        std::string const& power_action,
        std::string const& power_supply,
        int32_t power_action_timeout,
        pid_t pid);
    void dbus_SetLidBehavior(
        std::string const& sender,
        std::string const& power_action,
        std::string const& power_supply,
        pid_t pid);
    void dbus_SetCriticalPowerBehavior(
        std::string const& sender,
        std::string const& power_action,
        pid_t pid);

    void dbus_unknown_method(std::string const& sender, std::string const& name);
    pid_t dbus_get_invocation_sender_pid(GDBusMethodInvocation* invocation);

    std::shared_ptr<Log> const log;
    DBusConnectionHandle dbus_connection;
    DBusEventLoop dbus_event_loop;

    SetInactivityBehaviorHandler set_inactivity_behavior_handler;
    SetLidBehaviorHandler set_lid_behavior_handler;
    SetCriticalPowerBehaviorHandler set_critical_power_behavior_handler;

    // These need to be at the end, so that handlers are unregistered first on
    // destruction, to avoid accessing other members if an event arrives
    // on destruction.
    HandlerRegistration repowerd_handler_registration;
};

}
