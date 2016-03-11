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

class ClientRequests;
class DisplayPowerControl;
class PowerButton;
class PowerButtonEventSink;
class ProximitySensor;
class StateMachine;
class Timer;
class UserActivity;

class DaemonConfig
{
public:
    virtual ~DaemonConfig() = default;

    virtual std::shared_ptr<ClientRequests> the_client_requests() = 0;
    virtual std::shared_ptr<DisplayPowerControl> the_display_power_control() = 0;
    virtual std::shared_ptr<PowerButton> the_power_button() = 0;
    virtual std::shared_ptr<PowerButtonEventSink> the_power_button_event_sink() = 0;
    virtual std::shared_ptr<ProximitySensor> the_proximity_sensor() = 0;
    virtual std::shared_ptr<StateMachine> the_state_machine() = 0;
    virtual std::shared_ptr<Timer> the_timer() = 0;
    virtual std::shared_ptr<UserActivity> the_user_activity() = 0;

    virtual std::chrono::milliseconds power_button_long_press_timeout() = 0;
    virtual std::chrono::milliseconds user_inactivity_normal_display_off_timeout() = 0;
    virtual std::chrono::milliseconds user_inactivity_reduced_display_off_timeout() = 0;

protected:
    DaemonConfig() = default;
    DaemonConfig(DaemonConfig const&) = delete;
    DaemonConfig& operator=(DaemonConfig const&) = delete;
};

}
