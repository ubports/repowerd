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

#include "brightness_control.h"
#include "display_power_control.h"
#include "power_button_event_sink.h"
#include "proximity_sensor.h"
#include "timer.h"

repowerd::DefaultStateMachine::DefaultStateMachine(DaemonConfig& config)
    : brightness_control{config.the_brightness_control()},
      display_power_control{config.the_display_power_control()},
      power_button_event_sink{config.the_power_button_event_sink()},
      proximity_sensor{config.the_proximity_sensor()},
      timer{config.the_timer()},
      enable_inactivity_timeout{true},
      display_power_mode{DisplayPowerMode::off},
      display_power_mode_at_power_button_press{DisplayPowerMode::unknown},
      power_button_long_press_alarm_id{AlarmId::invalid},
      power_button_long_press_detected{false},
      power_button_long_press_timeout{config.power_button_long_press_timeout()},
      user_inactivity_display_dim_alarm_id{AlarmId::invalid},
      user_inactivity_display_off_alarm_id{AlarmId::invalid},
      user_inactivity_normal_display_dim_duration{
          config.user_inactivity_normal_display_dim_duration()},
      user_inactivity_normal_display_off_timeout{
          config.user_inactivity_normal_display_off_timeout()},
      user_inactivity_reduced_display_off_timeout{
          config.user_inactivity_reduced_display_off_timeout()}
{
}

void repowerd::DefaultStateMachine::handle_alarm(AlarmId id)
{
    if (id == power_button_long_press_alarm_id)
    {
        power_button_event_sink->notify_long_press();
        power_button_long_press_detected = true;
        power_button_long_press_alarm_id = AlarmId::invalid;
    }
    else if (id == user_inactivity_display_dim_alarm_id)
    {
        user_inactivity_display_dim_alarm_id = AlarmId::invalid;
        if (enable_inactivity_timeout)
            dim_display();
    }
    else if (id == user_inactivity_display_off_alarm_id)
    {
        user_inactivity_display_off_alarm_id = AlarmId::invalid;
        if (enable_inactivity_timeout)
            turn_off_display();
    }
}

void repowerd::DefaultStateMachine::handle_enable_inactivity_timeout()
{
    if (!enable_inactivity_timeout)
    {
        enable_inactivity_timeout = true;
        if (user_inactivity_display_off_alarm_id == AlarmId::invalid)
            turn_off_display();
    }
}

void repowerd::DefaultStateMachine::handle_disable_inactivity_timeout()
{
    enable_inactivity_timeout = false;
}

void repowerd::DefaultStateMachine::handle_notification()
{
    if (display_power_mode == DisplayPowerMode::on)
    {
        brighten_display();
        schedule_reduced_user_inactivity_alarm();
    }
    else if (proximity_sensor->proximity_state() == ProximityState::far)
    {
        turn_on_display_with_reduced_timeout();
    }
}

void repowerd::DefaultStateMachine::handle_power_button_press()
{
    display_power_mode_at_power_button_press = display_power_mode;

    if (display_power_mode == DisplayPowerMode::off)
    {
        turn_on_display_with_normal_timeout();
    }

    power_button_long_press_alarm_id =
        timer->schedule_alarm_in(power_button_long_press_timeout);
}

void repowerd::DefaultStateMachine::handle_power_button_release()
{
    if (power_button_long_press_detected)
    {
        power_button_long_press_detected = false;
    }
    else if (display_power_mode_at_power_button_press == DisplayPowerMode::on)
    {
        turn_off_display();
    }

    display_power_mode_at_power_button_press = DisplayPowerMode::unknown;
    power_button_long_press_alarm_id = AlarmId::invalid;
}

void repowerd::DefaultStateMachine::handle_proximity_far()
{
    if (display_power_mode == DisplayPowerMode::off)
        turn_on_display_with_normal_timeout();
}

void repowerd::DefaultStateMachine::handle_proximity_near()
{
    if (display_power_mode == DisplayPowerMode::on)
        turn_off_display();
}

void repowerd::DefaultStateMachine::handle_user_activity_changing_power_state()
{
    if (display_power_mode == DisplayPowerMode::on)
    {
        brighten_display();
        schedule_normal_user_inactivity_alarm();
    }
    else if (proximity_sensor->proximity_state() == ProximityState::far)
    {
        turn_on_display_with_normal_timeout();
    }
}

void repowerd::DefaultStateMachine::handle_user_activity_extending_power_state()
{
    if (display_power_mode == DisplayPowerMode::on)
    {
        brighten_display();
        schedule_normal_user_inactivity_alarm();
    }
}

void repowerd::DefaultStateMachine::cancel_user_inactivity_alarm()
{
    user_inactivity_display_dim_alarm_id = AlarmId::invalid;
    user_inactivity_display_off_alarm_id = AlarmId::invalid;
    user_inactivity_display_off_time_point = {};
}

void repowerd::DefaultStateMachine::schedule_normal_user_inactivity_alarm()
{
    cancel_user_inactivity_alarm();
    user_inactivity_display_off_time_point =
        timer->now() + user_inactivity_normal_display_off_timeout;
    user_inactivity_display_dim_alarm_id =
        timer->schedule_alarm_in(
            user_inactivity_normal_display_off_timeout -
            user_inactivity_normal_display_dim_duration);
    user_inactivity_display_off_alarm_id =
        timer->schedule_alarm_in(user_inactivity_normal_display_off_timeout);
}

void repowerd::DefaultStateMachine::schedule_reduced_user_inactivity_alarm()
{
    auto const tp = timer->now() + user_inactivity_reduced_display_off_timeout;
    if (tp > user_inactivity_display_off_time_point)
    {
        cancel_user_inactivity_alarm();
        user_inactivity_display_off_alarm_id =
            timer->schedule_alarm_in(user_inactivity_reduced_display_off_timeout);
        user_inactivity_display_off_time_point = tp;
    }
}

void repowerd::DefaultStateMachine::turn_off_display()
{
    brightness_control->set_off_brightness();
    display_power_control->turn_off();
    display_power_mode = DisplayPowerMode::off;
    cancel_user_inactivity_alarm();
}

void repowerd::DefaultStateMachine::turn_on_display_with_normal_timeout()
{
    display_power_control->turn_on();
    display_power_mode = DisplayPowerMode::on;
    brighten_display();
    schedule_normal_user_inactivity_alarm();
}

void repowerd::DefaultStateMachine::turn_on_display_with_reduced_timeout()
{
    display_power_control->turn_on();
    brightness_control->set_normal_brightness();
    display_power_mode = DisplayPowerMode::on;
    schedule_reduced_user_inactivity_alarm();
}

void repowerd::DefaultStateMachine::brighten_display()
{
    brightness_control->set_normal_brightness();
}

void repowerd::DefaultStateMachine::dim_display()
{
    brightness_control->set_dim_brightness();
}
