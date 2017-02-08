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
#include "power_action.h"
#include "power_supply.h"

#include <chrono>

namespace repowerd
{

class StateMachine
{
public:
    virtual ~StateMachine() = default;

    virtual void handle_alarm(AlarmId id) = 0;

    virtual void handle_active_call() = 0;
    virtual void handle_no_active_call() = 0;

    virtual void handle_enable_inactivity_timeout() = 0;
    virtual void handle_disable_inactivity_timeout() = 0;
    virtual void handle_set_inactivity_behavior(
        PowerAction power_action,
        PowerSupply power_supply,
        std::chrono::milliseconds timeout) = 0;

    virtual void handle_lid_closed() = 0;
    virtual void handle_lid_open() = 0;
    virtual void handle_set_lid_behavior(
        PowerAction power_action,
        PowerSupply power_supply) = 0;

    virtual void handle_no_notification() = 0;
    virtual void handle_notification() = 0;

    virtual void handle_power_button_press() = 0;
    virtual void handle_power_button_release() = 0;

    virtual void handle_power_source_change() = 0;
    virtual void handle_power_source_critical() = 0;

    virtual void handle_proximity_far() = 0;
    virtual void handle_proximity_near() = 0;

    virtual void handle_user_activity_changing_power_state() = 0;
    virtual void handle_user_activity_extending_power_state() = 0;

    virtual void handle_set_normal_brightness_value(double) = 0;
    virtual void handle_enable_autobrightness() = 0;
    virtual void handle_disable_autobrightness() = 0;

    virtual void handle_system_resume() = 0;

    virtual void start() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;

protected:
    StateMachine() = default;
    StateMachine(StateMachine const&) = delete;
    StateMachine& operator=(StateMachine const&) = delete;
};

}

