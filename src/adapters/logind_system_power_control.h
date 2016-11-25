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
#include "fd.h"

#include <memory>
#include <mutex>
#include <unordered_map>

namespace repowerd
{
class Log;

class LogindSystemPowerControl : public SystemPowerControl
{
public:
    LogindSystemPowerControl(
        std::shared_ptr<Log> const& log,
        std::string const& dbus_bus_address);

    void allow_suspend(std::string const& id) override;
    void disallow_suspend(std::string const& id) override;
    void power_off() override;

private:
    std::shared_ptr<Log> const log;
    DBusConnectionHandle dbus_connection;

    Fd dbus_inhibit(char const* what, char const* who);
    void dbus_power_off();

    std::mutex inhibitions_mutex;
    std::unordered_map<std::string, Fd> suspend_disallowances;
};

}
