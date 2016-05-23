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

#include "src/core/performance_booster.h"

#include <ubuntu/hardware/booster.h>

#include <memory>

namespace repowerd
{

class Log;

class UbuntuPerformanceBooster : public PerformanceBooster
{
public:
    UbuntuPerformanceBooster(std::shared_ptr<Log> const& log);
    ~UbuntuPerformanceBooster();

    void enable_interactive_mode() override;
    void disable_interactive_mode() override;

private:
    std::shared_ptr<Log> const log;
    std::unique_ptr<UHardwareBooster,void(*)(UHardwareBooster*)> booster;
};

}
