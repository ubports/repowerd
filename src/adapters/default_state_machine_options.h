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

#include "src/core/state_machine_options.h"

#include <deviceinfo.h>

#include <string>
#include <memory>

class DeviceInfo;

namespace repowerd
{
class Log;

class DefaultStateMachineOptions : public StateMachineOptions
{
public:
    DefaultStateMachineOptions(Log& log);

    std::chrono::milliseconds notification_expiration_timeout() const override;
    std::chrono::milliseconds power_button_long_press_timeout() const override;
    std::chrono::milliseconds user_inactivity_normal_display_dim_duration() const override;
    std::chrono::milliseconds user_inactivity_normal_display_off_timeout() const override;
    std::chrono::milliseconds user_inactivity_normal_suspend_timeout() const override;
    std::chrono::milliseconds user_inactivity_post_notification_display_off_timeout() const override;
    std::chrono::milliseconds user_inactivity_reduced_display_off_timeout() const override;

    bool treat_power_button_as_user_activity() const override;
    bool turn_on_display_at_startup() const override;

private:
    std::unique_ptr<DeviceInfo> deviceinfo;
};

}
