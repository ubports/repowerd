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

#include <memory>
#include <chrono>

namespace repowerd
{

class StateMachineOptions
{
public:
    virtual ~StateMachineOptions() = default;

    virtual std::chrono::milliseconds notification_expiration_timeout() const = 0;
    virtual std::chrono::milliseconds power_button_long_press_timeout() const = 0;
    virtual std::chrono::milliseconds user_inactivity_normal_display_dim_duration() const = 0;
    virtual std::chrono::milliseconds user_inactivity_normal_display_off_timeout() const = 0;
    virtual std::chrono::milliseconds user_inactivity_normal_suspend_timeout() const = 0;
    virtual std::chrono::milliseconds user_inactivity_post_notification_display_off_timeout() const = 0;
    virtual std::chrono::milliseconds user_inactivity_reduced_display_off_timeout() const = 0;

    virtual bool treat_power_button_as_user_activity() const = 0;
    virtual bool turn_on_display_at_startup() const = 0;

protected:
    StateMachineOptions() = default;
    StateMachineOptions(StateMachineOptions const&) = delete;
    StateMachineOptions& operator=(StateMachineOptions const&) = delete;
};

}
