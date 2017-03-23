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

#include "state_machine.h"

namespace repowerd
{

class NullStateMachine : public StateMachine
{
public:
    void handle_alarm(AlarmId) override {}

    void handle_active_call() override {}
    void handle_no_active_call() override {}

    void handle_enable_inactivity_timeout() override {}
    void handle_disable_inactivity_timeout() override {}
    void handle_set_inactivity_behavior(
        PowerAction, PowerSupply, std::chrono::milliseconds) override
    {
    }

    void handle_lid_closed() override {}
    void handle_lid_open() override {}
    void handle_set_lid_behavior(PowerAction, PowerSupply) override {}

    void handle_no_notification() override {}
    void handle_notification() override {}

    void handle_power_button_press() override {}
    void handle_power_button_release() override {}

    void handle_power_source_change() override {}
    void handle_power_source_critical() override {}
    void handle_set_critical_power_behavior(PowerAction) override {}

    void handle_proximity_far() override {}
    void handle_proximity_near() override {}

    void handle_user_activity_changing_power_state() override {}
    void handle_user_activity_extending_power_state() override {}

    void handle_set_normal_brightness_value(double) override {}
    void handle_enable_autobrightness() override {}
    void handle_disable_autobrightness() override {}

    void handle_allow_suspend() override {}
    void handle_disallow_suspend() override {}

    void handle_system_resume() override {}

    void start() override {}
    void pause() override {}
    void resume() override {}
};

}
