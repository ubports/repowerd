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

#include "default_state_machine.h"

#include "display_power_control.h"
#include "power_button_event_sink.h"
#include "timer.h"

using namespace std::chrono_literals;

repowerd::DefaultStateMachine::DefaultStateMachine(DaemonConfig& config)
    : display_power_control{config.the_display_power_control()},
      power_button_event_sink{config.the_power_button_event_sink()},
      timer{config.the_timer()},
      display_power_mode{DisplayPowerMode::off},
      display_power_mode_at_power_button_press{DisplayPowerMode::unknown},
      long_press_alarm_id{AlarmId::invalid},
      long_press_detected{false}
{
}

void repowerd::DefaultStateMachine::handle_power_button_press()
{
    display_power_mode_at_power_button_press = display_power_mode;

    if (display_power_mode == DisplayPowerMode::off)
    {
        display_power_control->turn_on();
        display_power_mode = DisplayPowerMode::on;
    }

    long_press_alarm_id = timer->schedule_alarm_in(2s);
}

void repowerd::DefaultStateMachine::handle_power_button_release()
{
    if (long_press_detected)
    {
        power_button_event_sink->notify_long_press();
        long_press_detected = false;
    }
    else if (display_power_mode_at_power_button_press == DisplayPowerMode::on)
    {
        display_power_control->turn_off();
        display_power_mode = DisplayPowerMode::off;
    }

    display_power_mode_at_power_button_press = DisplayPowerMode::unknown;
    long_press_alarm_id = AlarmId::invalid;
}

void repowerd::DefaultStateMachine::handle_alarm(AlarmId id)
{
    if (long_press_alarm_id == id)
    {
        long_press_detected = true;
        long_press_alarm_id = AlarmId::invalid;
    }
}
