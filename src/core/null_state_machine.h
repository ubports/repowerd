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
    void handle_alarm(AlarmId) {}

    void handle_active_call() {}
    void handle_no_active_call() {}

    void handle_enable_inactivity_timeout() {}
    void handle_disable_inactivity_timeout() {}
    void handle_set_inactivity_timeout(std::chrono::milliseconds) {}

    void handle_no_notification() {}
    void handle_notification() {}

    void handle_power_button_press() {}
    void handle_power_button_release() {}

    void handle_power_source_change() {}
    void handle_power_source_critical() {}

    void handle_proximity_far() {}
    void handle_proximity_near() {}

    void handle_turn_on_display() {}

    void handle_user_activity_changing_power_state() {}
    void handle_user_activity_extending_power_state() {}

    void handle_set_normal_brightness_value(double) {}
    void handle_enable_autobrightness() {}
    void handle_disable_autobrightness() {}
};

}
