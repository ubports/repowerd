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

repowerd::DefaultStateMachine::DefaultStateMachine(DaemonConfig& config)
    : display_power_control{config.the_display_power_control()},
      display_power_mode{DisplayPowerMode::off},
      display_power_mode_at_power_key_press{DisplayPowerMode::unknown}
{
}

void repowerd::DefaultStateMachine::handle_power_key_press()
{
    display_power_mode_at_power_key_press = display_power_mode;

    if (display_power_mode == DisplayPowerMode::off)
    {
        display_power_control->turn_on();
        display_power_mode = DisplayPowerMode::on;
    }
}

void repowerd::DefaultStateMachine::handle_power_key_release()
{
    if (display_power_mode_at_power_key_press == DisplayPowerMode::on)
    {
        display_power_control->turn_off();
        display_power_mode = DisplayPowerMode::off;
    }

    display_power_mode_at_power_key_press = DisplayPowerMode::unknown;
}
