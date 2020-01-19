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
#include "src/core/log.h"
#include "src/core/infinite_timeout.h"

#include <deviceinfo.h>

using namespace std::chrono_literals;

char const* const log_tag = "DefaultStateMachineOptions";

repowerd::DefaultStateMachineOptions::DefaultStateMachineOptions(
    repowerd::Log& log)
    : deviceinfo(std::make_unique<DeviceInfo>())
{
    log.log(log_tag, "DeviceName: %s", deviceinfo->name().c_str());
    log.log(log_tag, "Option: notification_expiration_timeout=%lld",
            static_cast<long long>(notification_expiration_timeout().count()));
    log.log(log_tag, "Option: power_button_long_press_timeout=%lld",
            static_cast<long long>(power_button_long_press_timeout().count()));
    log.log(log_tag, "Option: treat_power_button_as_user_activity=%s",
            treat_power_button_as_user_activity() ? "true" : "false");
    log.log(log_tag, "Option: turn_on_display_at_startup=%s",
            turn_on_display_at_startup() ? "true" : "false");
    log.log(log_tag, "Option: user_inactivity_normal_display_dim_duration=%lld",
            static_cast<long long>(user_inactivity_normal_display_dim_duration().count()));
    log.log(log_tag, "Option: user_inactivity_normal_display_off_timeout=%lld",
            static_cast<long long>(user_inactivity_normal_display_off_timeout().count()));
    log.log(log_tag, "Option: user_inactivity_normal_suspend_timeout=%lld",
            static_cast<long long>(user_inactivity_normal_suspend_timeout().count()));
    log.log(log_tag, "Option: user_inactivity_post_notification_display_off_timeout=%lld",
            static_cast<long long>(user_inactivity_post_notification_display_off_timeout().count()));
    log.log(log_tag, "Option: user_inactivity_reduced_display_off_timeout=%lld",
            static_cast<long long>(user_inactivity_reduced_display_off_timeout().count()));
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
repowerd::DefaultStateMachineOptions::user_inactivity_normal_suspend_timeout() const
{
    return repowerd::infinite_timeout;
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
    return deviceinfo->deviceType() == DeviceInfo::Desktop;
}

bool repowerd::DefaultStateMachineOptions::turn_on_display_at_startup() const
{
    return true;
}
