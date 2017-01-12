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

class DisplayInformation;
class BrightnessControl;
class ClientRequests;
class DisplayPowerControl;
class DisplayPowerEventSink;
class Lid;
class Log;
class ModemPowerControl;
class NotificationService;
class PerformanceBooster;
class PowerButton;
class PowerButtonEventSink;
class PowerSource;
class ProximitySensor;
class SessionTracker;
class StateMachineFactory;
class StateMachineOptions;
class SystemPowerControl;
class Timer;
class UserActivity;
class VoiceCallService;

class DaemonConfig
{
public:
    virtual ~DaemonConfig() = default;

    virtual std::shared_ptr<DisplayInformation> the_display_information() = 0;
    virtual std::shared_ptr<BrightnessControl> the_brightness_control() = 0;
    virtual std::shared_ptr<ClientRequests> the_client_requests() = 0;
    virtual std::shared_ptr<DisplayPowerControl> the_display_power_control() = 0;
    virtual std::shared_ptr<DisplayPowerEventSink> the_display_power_event_sink() = 0;
    virtual std::shared_ptr<Lid> the_lid() = 0;
    virtual std::shared_ptr<Log> the_log() = 0;
    virtual std::shared_ptr<ModemPowerControl> the_modem_power_control() = 0;
    virtual std::shared_ptr<NotificationService> the_notification_service() = 0;
    virtual std::shared_ptr<PerformanceBooster> the_performance_booster() = 0;
    virtual std::shared_ptr<PowerButton> the_power_button() = 0;
    virtual std::shared_ptr<PowerButtonEventSink> the_power_button_event_sink() = 0;
    virtual std::shared_ptr<PowerSource> the_power_source() = 0;
    virtual std::shared_ptr<ProximitySensor> the_proximity_sensor() = 0;
    virtual std::shared_ptr<SessionTracker> the_session_tracker() = 0;
    virtual std::shared_ptr<StateMachineFactory> the_state_machine_factory() = 0;
    virtual std::shared_ptr<StateMachineOptions> the_state_machine_options() = 0;
    virtual std::shared_ptr<SystemPowerControl> the_system_power_control() = 0;
    virtual std::shared_ptr<Timer> the_timer() = 0;
    virtual std::shared_ptr<UserActivity> the_user_activity() = 0;
    virtual std::shared_ptr<VoiceCallService> the_voice_call_service() = 0;

protected:
    DaemonConfig() = default;
    DaemonConfig(DaemonConfig const&) = delete;
    DaemonConfig& operator=(DaemonConfig const&) = delete;
};

}
