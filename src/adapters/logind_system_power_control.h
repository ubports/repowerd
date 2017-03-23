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

#pragma once

#include "src/core/system_power_control.h"

#include "dbus_connection_handle.h"
#include "dbus_event_loop.h"
#include "fd.h"

#include <memory>
#include <mutex>
#include <unordered_set>

namespace repowerd
{
class Log;

class LogindSystemPowerControl : public SystemPowerControl
{
public:
    LogindSystemPowerControl(
        std::shared_ptr<Log> const& log,
        std::string const& dbus_bus_address);

    void start_processing() override;
    HandlerRegistration register_system_resume_handler(
        SystemResumeHandler const& system_resume_handler) override;

    void allow_suspend(std::string const& id, SuspendType suspend_type) override;
    void disallow_suspend(std::string const& id, SuspendType suspend_type) override;

    void power_off() override;
    void suspend() override;
    void suspend_if_allowed() override;
    void suspend_when_allowed(std::string const& id) override;
    void cancel_suspend_when_allowed(std::string const& id) override;

    void allow_default_system_handlers() override;
    void disallow_default_system_handlers() override;

private:
    void handle_dbus_signal(
        GDBusConnection* connection,
        gchar const* sender,
        gchar const* object_path,
        gchar const* interface_name,
        gchar const* signal_name,
        GVariant* parameters);
    Fd dbus_inhibit(char const* what, char const* who);
    void dbus_power_off();
    void dbus_suspend();

    std::shared_ptr<Log> const log;
    DBusConnectionHandle dbus_connection;
    DBusEventLoop dbus_event_loop;

    HandlerRegistration dbus_manager_signal_handler_registration;
    SystemResumeHandler system_resume_handler;

    std::mutex inhibitions_mutex;
    std::unordered_set<std::string> suspend_disallowances;
    std::unordered_set<std::string> pending_suspends;
    Fd idle_and_lid_inhibition_fd;
};

}
