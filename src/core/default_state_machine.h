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

#include "state_machine.h"
#include "daemon_config.h"
#include "display_power_change_reason.h"

#include <array>

namespace repowerd
{

class DefaultStateMachine : public StateMachine
{
public:
    DefaultStateMachine(DaemonConfig& config);

    void handle_alarm(AlarmId id) override;

    void handle_active_call() override;
    void handle_no_active_call() override;

    void handle_enable_inactivity_timeout() override;
    void handle_disable_inactivity_timeout() override;
    void handle_set_inactivity_timeout(std::chrono::milliseconds timeout) override;

    void handle_no_notification() override;
    void handle_notification() override;

    void handle_power_button_press() override;
    void handle_power_button_release() override;

    void handle_power_source_change() override;
    void handle_power_source_critical() override;

    void handle_proximity_far() override;
    void handle_proximity_near() override;

    void handle_turn_on_display() override;

    void handle_user_activity_changing_power_state() override;
    void handle_user_activity_extending_power_state() override;

private:
    enum class DisplayPowerMode {unknown, on, off};
    struct InactivityTimeoutAllowanceEnum {
        enum Allowance {client, notification, count};
    };
    using InactivityTimeoutAllowance = InactivityTimeoutAllowanceEnum::Allowance;
    struct ProximityEnablementEnum {
        enum Enablement {until_far_event, until_disabled, until_far_event_or_timeout, count};
    };
    using ProximityEnablement = ProximityEnablementEnum::Enablement;
    enum class ScheduledTimeoutType {none, normal, post_notification, reduced};

    void cancel_user_inactivity_alarm();
    void schedule_normal_user_inactivity_alarm();
    void schedule_post_notification_user_inactivity_alarm();
    void schedule_reduced_user_inactivity_alarm();
    void schedule_proximity_disable_alarm();
    void turn_off_display(DisplayPowerChangeReason reason);
    void turn_on_display_without_timeout(DisplayPowerChangeReason reason);
    void turn_on_display_with_normal_timeout(DisplayPowerChangeReason reason);
    void turn_on_display_with_reduced_timeout(DisplayPowerChangeReason reason);
    void brighten_display();
    void dim_display();
    void allow_inactivity_timeout(InactivityTimeoutAllowance allowance);
    void disallow_inactivity_timeout(InactivityTimeoutAllowance allowance);
    bool is_inactivity_timeout_allowed();
    bool is_inactivity_timeout_application_allowed();
    void enable_proximity(ProximityEnablement enablement);
    void disable_proximity(ProximityEnablement enablement);
    bool is_proximity_enabled();
    bool is_proximity_enabled_only_until_far_event();

    std::shared_ptr<BrightnessControl> const brightness_control;
    std::shared_ptr<DisplayPowerControl> const display_power_control;
    std::shared_ptr<DisplayPowerEventSink> const display_power_event_sink;
    std::shared_ptr<Log> const log;
    std::shared_ptr<ModemPowerControl> const modem_power_control;
    std::shared_ptr<PerformanceBooster> const performance_booster;
    std::shared_ptr<PowerButtonEventSink> const power_button_event_sink;
    std::shared_ptr<ProximitySensor> const proximity_sensor;
    std::shared_ptr<ShutdownControl> const shutdown_control;
    std::shared_ptr<SuspendControl> const suspend_control;
    std::shared_ptr<Timer> const timer;

    std::array<bool,InactivityTimeoutAllowance::count> inactivity_timeout_allowances;
    std::array<bool,ProximityEnablement::count> proximity_enablements;
    DisplayPowerMode display_power_mode;
    DisplayPowerMode display_power_mode_at_power_button_press;
    DisplayPowerChangeReason display_power_mode_reason;
    AlarmId power_button_long_press_alarm_id;
    bool power_button_long_press_detected;
    std::chrono::milliseconds power_button_long_press_timeout;
    AlarmId user_inactivity_display_dim_alarm_id;
    AlarmId user_inactivity_display_off_alarm_id;
    AlarmId proximity_disable_alarm_id;
    std::chrono::steady_clock::time_point user_inactivity_display_off_time_point;
    std::chrono::milliseconds const user_inactivity_normal_display_dim_duration;
    std::chrono::milliseconds user_inactivity_normal_display_off_timeout;
    std::chrono::milliseconds const user_inactivity_reduced_display_off_timeout;
    std::chrono::milliseconds const user_inactivity_post_notification_display_off_timeout;
    ScheduledTimeoutType scheduled_timeout_type;
};

}
