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

#include "ubuntu_performance_booster.h"

#include "src/core/log.h"

namespace
{

char const* const log_tag = "UbuntuPerformanceBooster";

void u_hardware_booster_deleter(UHardwareBooster* booster)
{
    if (booster) u_hardware_booster_unref(booster);
}

}

repowerd::UbuntuPerformanceBooster::UbuntuPerformanceBooster(
    std::shared_ptr<Log> const& log)
    : log{log},
      booster{u_hardware_booster_new(), u_hardware_booster_deleter}
{
    if (!booster)
        throw std::runtime_error{"Failed to create ubuntu performance booster"};
}

repowerd::UbuntuPerformanceBooster::~UbuntuPerformanceBooster() = default;

void repowerd::UbuntuPerformanceBooster::enable_interactive_mode()
{
    log->log(log_tag, "enable_interactive_mode()");

    u_hardware_booster_enable_scenario(
        booster.get(),
        U_HARDWARE_BOOSTER_SCENARIO_USER_INTERACTION);
}

void repowerd::UbuntuPerformanceBooster::disable_interactive_mode()
{
    log->log(log_tag, "disable_interactive_mode()");

    u_hardware_booster_disable_scenario(
        booster.get(),
        U_HARDWARE_BOOSTER_SCENARIO_USER_INTERACTION);
}
