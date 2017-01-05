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

#include "real_temporary_suspend_inhibition.h"

#include "src/core/system_power_control.h"

repowerd::RealTemporarySuspendInhibition::RealTemporarySuspendInhibition(
    std::shared_ptr<SystemPowerControl> const& system_power_control)
    : system_power_control{system_power_control},
      event_loop{"TempSuspendInhibit"},
      id{0}
{
}

void repowerd::RealTemporarySuspendInhibition::inhibit_suspend_for(
    std::chrono::milliseconds timeout, std::string const& name)
{
    auto const cur_id = id++;
    auto const suspend_id = name + "_" + std::to_string(cur_id);

    system_power_control->disallow_suspend(suspend_id, SuspendType::automatic);

    event_loop.schedule_in(
        timeout,
        [this, suspend_id]
        {
            system_power_control->allow_suspend(suspend_id, SuspendType::automatic);
        });
}
