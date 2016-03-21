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

#include "daemon_config.h"

namespace repowerd
{

class DefaultDaemonConfig : public DaemonConfig
{
public:
    std::shared_ptr<BrightnessControl> the_brightness_control() override;
    std::shared_ptr<ClientRequests> the_client_requests() override;
    std::shared_ptr<DisplayPowerControl> the_display_power_control() override;
    std::shared_ptr<NotificationService> the_notification_service() override;
    std::shared_ptr<PowerButton> the_power_button() override;
    std::shared_ptr<PowerButtonEventSink> the_power_button_event_sink() override;
    std::shared_ptr<ProximitySensor> the_proximity_sensor() override;
    std::shared_ptr<StateMachine> the_state_machine() override;
    std::shared_ptr<Timer> the_timer() override;
    std::shared_ptr<UserActivity> the_user_activity() override;

    std::chrono::milliseconds power_button_long_press_timeout() override;
    std::chrono::milliseconds user_inactivity_normal_display_dim_duration() override;
    std::chrono::milliseconds user_inactivity_normal_display_off_timeout() override;
    std::chrono::milliseconds user_inactivity_reduced_display_off_timeout() override;

private:
    std::shared_ptr<BrightnessControl> brightness_control;
    std::shared_ptr<ClientRequests> client_requests;
    std::shared_ptr<DisplayPowerControl> display_power_control;
    std::shared_ptr<NotificationService> notification_service;
    std::shared_ptr<PowerButton> power_button;
    std::shared_ptr<PowerButtonEventSink> power_button_event_sink;
    std::shared_ptr<ProximitySensor> proximity_sensor;
    std::shared_ptr<StateMachine> state_machine;
    std::shared_ptr<Timer> timer;
    std::shared_ptr<UserActivity> user_activity;
};

}

