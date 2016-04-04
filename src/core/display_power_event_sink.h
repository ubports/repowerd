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

#include "display_power_change_reason.h"

namespace repowerd
{

class DisplayPowerEventSink
{
public:
    virtual ~DisplayPowerEventSink() = default;

    virtual void notify_display_power_off(DisplayPowerChangeReason reason) = 0;
    virtual void notify_display_power_on(DisplayPowerChangeReason reason) = 0;

protected:
    DisplayPowerEventSink() = default;
    DisplayPowerEventSink(DisplayPowerEventSink const&) = delete;
    DisplayPowerEventSink& operator=(DisplayPowerEventSink const&) = delete;
};

}
