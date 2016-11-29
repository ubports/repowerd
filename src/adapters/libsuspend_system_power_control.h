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

#include <memory>
#include <mutex>
#include <unordered_set>

namespace repowerd
{
class Log;

class LibsuspendSystemPowerControl : public SystemPowerControl
{
public:
    LibsuspendSystemPowerControl(std::shared_ptr<Log> const& log);

    void allow_suspend(std::string const& id, SuspendType suspend_type) override;
    void disallow_suspend(std::string const& id, SuspendType suspend_type) override;

    void power_off() override;

private:
    std::shared_ptr<Log> const log;

    std::mutex suspend_mutex;
    std::unordered_set<std::string> suspend_disallowances;
};

}
