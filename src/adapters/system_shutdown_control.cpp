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

#include "system_shutdown_control.h"
#include "src/core/log.h"

#include <cstdlib>

repowerd::SystemShutdownControl::SystemShutdownControl(
    std::shared_ptr<Log> const& log)
    : log{log}
{
}

void repowerd::SystemShutdownControl::power_off()
{
    log->log("SystemShutdownControl", "power_off()");

    if (system("shutdown -P now")) {}
}
