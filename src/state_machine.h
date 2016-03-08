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

#include "alarm_id.h"

namespace repowerd
{

class StateMachine
{
public:
    virtual ~StateMachine() = default;

    virtual void handle_power_button_press() = 0;
    virtual void handle_power_button_release() = 0;
    virtual void handle_alarm(AlarmId id) = 0;

protected:
    StateMachine() = default;
    StateMachine(StateMachine const&) = delete;
    StateMachine& operator=(StateMachine const&) = delete;
};

}

