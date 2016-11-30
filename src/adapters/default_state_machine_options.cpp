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

#include <hybris/properties/properties.h>

using namespace std::chrono_literals;

namespace
{

std::string determine_device_name()
{
    char name[PROP_VALUE_MAX] = "";
    property_get("ro.product.device", name, "");
    return name;
}

bool treat_power_button_as_user_activity_for(
    std::string const& device_name)
{
    // On non-phablet/non-android devices the power button
    // is treated as a generic power-state-changing user activity, i.e.,
    // it can turn on the screen but not turn it off
    return device_name.empty();
}

}

repowerd::DefaultStateMachineOptions::DefaultStateMachineOptions()
    : treat_power_button_as_user_activity_{
        treat_power_button_as_user_activity_for(determine_device_name())}
{
}

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

bool repowerd::DefaultStateMachineOptions::treat_power_button_as_user_activity() const
{
    return treat_power_button_as_user_activity_;
}

bool repowerd::DefaultStateMachineOptions::turn_on_display_at_startup() const
{
    return true;
}
