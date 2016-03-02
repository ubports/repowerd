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
      is_display_on{false}
{
}

void repowerd::DefaultStateMachine::handle_power_key_press()
{
    if (is_display_on)
    {
        display_power_control->turn_off();
        is_display_on = false;
    }
    else
    {
        display_power_control->turn_on();
        is_display_on = true;
    }
}

void repowerd::DefaultStateMachine::handle_power_key_release()
{
}
