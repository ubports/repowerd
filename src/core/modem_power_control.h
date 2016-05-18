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

namespace repowerd
{

class ModemPowerControl
{
public:
    virtual ~ModemPowerControl() = default;

    virtual void set_low_power_mode() = 0;
    virtual void set_normal_power_mode() = 0;

protected:
    ModemPowerControl() = default;
    ModemPowerControl (ModemPowerControl const&) = default;
    ModemPowerControl& operator=(ModemPowerControl const&) = default;
};

}
