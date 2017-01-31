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

#include "fake_state_machine_options.h"
#include "src/core/infinite_timeout.h"

using namespace std::chrono_literals;
namespace rt = repowerd::test;

std::chrono::milliseconds
rt::FakeStateMachineOptions::notification_expiration_timeout() const
{
    return 60s;
}

std::chrono::milliseconds
rt::FakeStateMachineOptions::power_button_long_press_timeout() const
{
    return 2s;
}

std::chrono::milliseconds
rt::FakeStateMachineOptions::user_inactivity_normal_display_dim_duration() const
{
    return 10s;
}

std::chrono::milliseconds
rt::FakeStateMachineOptions::user_inactivity_normal_display_off_timeout() const
{
    return 60s;
}

std::chrono::milliseconds
rt::FakeStateMachineOptions::user_inactivity_normal_suspend_timeout() const
{
    return 120s;
}

std::chrono::milliseconds
rt::FakeStateMachineOptions::user_inactivity_post_notification_display_off_timeout() const
{
    return 5s;
}

std::chrono::milliseconds
rt::FakeStateMachineOptions::user_inactivity_reduced_display_off_timeout() const
{
    return 10s;
}

bool rt::FakeStateMachineOptions::treat_power_button_as_user_activity() const
{
    return false;
}

bool rt::FakeStateMachineOptions::turn_on_display_at_startup() const
{
    return false;
}
