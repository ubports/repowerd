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

#include "libsuspend_suspend_control.h"
#include "libsuspend/libsuspend.h"

#include "src/core/log.h"

namespace
{
char const* const log_tag = "LibsuspendSuspendControl";
}

repowerd::LibsuspendSuspendControl::LibsuspendSuspendControl(
    std::shared_ptr<Log> const& log)
    : log{log}
{
    libsuspend_init(0);
    log->log(log_tag, "Initialized using backend %s", libsuspend_getname());
}

void repowerd::LibsuspendSuspendControl::allow_suspend(std::string const& id)
{
    std::lock_guard<std::mutex> lock{suspend_mutex};

    log->log(log_tag, "allow_suspend(%s)", id.c_str());

    if (suspend_disallowances.erase(id) > 0 &&
        suspend_disallowances.empty())
    {
        log->log(log_tag, "Preparing for suspend");
        libsuspend_prepare_suspend();
        libsuspend_enter_suspend();
    }
}

void repowerd::LibsuspendSuspendControl::disallow_suspend(std::string const& id)
{
    std::lock_guard<std::mutex> lock{suspend_mutex};

    log->log(log_tag, "disallow_suspend(%s)", id.c_str());

    auto const could_be_suspended = suspend_disallowances.empty();

    suspend_disallowances.insert(id);

    if (could_be_suspended)
    {
        log->log(log_tag, "exiting suspend");
        libsuspend_exit_suspend();
    }
}
