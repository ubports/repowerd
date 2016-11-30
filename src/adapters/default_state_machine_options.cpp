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

#include "default_state_machine_options.h"

using namespace std::chrono_literals;

std::chrono::milliseconds
repowerd::DefaultStateMachineOptions::notification_expiration_timeout() const
{
    return 60s;
}

std::chrono::milliseconds
repowerd::DefaultStateMachineOptions::power_button_long_press_timeout() const
{
    return 2s;
}

std::chrono::milliseconds
repowerd::DefaultStateMachineOptions::user_inactivity_normal_display_dim_duration() const
{
    return 10s;
}

std::chrono::milliseconds
repowerd::DefaultStateMachineOptions::user_inactivity_normal_display_off_timeout() const
{
    return 60s;
}

std::chrono::milliseconds
repowerd::DefaultStateMachineOptions::user_inactivity_post_notification_display_off_timeout() const
{
    return 5s;
}

std::chrono::milliseconds
repowerd::DefaultStateMachineOptions::user_inactivity_reduced_display_off_timeout() const
{
    return 10s;
}

bool repowerd::DefaultStateMachineOptions::turn_on_display_at_startup() const
{
    return true;
}
