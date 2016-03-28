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

#include "default_daemon_config.h"
#include "default_state_machine.h"

#include "brightness_control.h"
#include "client_requests.h"
#include "display_power_control.h"
#include "notification_service.h"
#include "power_button.h"
#include "power_button_event_sink.h"
#include "proximity_sensor.h"
#include "timer.h"
#include "user_activity.h"
#include "voice_call_service.h"

using namespace std::chrono_literals;

namespace
{

struct NullHandlerRegistration : repowerd::HandlerRegistration
{
    NullHandlerRegistration() : HandlerRegistration{[]{}} {}
};

struct NullBrightnessControl : repowerd::BrightnessControl
{
    void disable_autobrightness() override {}
    void enable_autobrightness() override {}
    void set_dim_brightness() override {}
    void set_normal_brightness() override {}
    void set_normal_brightness_value(float)  override {}
    void set_off_brightness() override {}
};

struct NullClientRequests : repowerd::ClientRequests
{
    repowerd::HandlerRegistration register_disable_inactivity_timeout_handler(
        repowerd::DisableInactivityTimeoutHandler const&)
    {
        return NullHandlerRegistration{};
    }

    repowerd::HandlerRegistration register_enable_inactivity_timeout_handler(
        repowerd::EnableInactivityTimeoutHandler const&)
    {
        return NullHandlerRegistration{};
    }

    repowerd::HandlerRegistration register_disable_autobrightness_handler(
        repowerd::DisableAutobrightnessHandler const&)
    {
        return NullHandlerRegistration{};
    }

    repowerd::HandlerRegistration register_enable_autobrightness_handler(
        repowerd::EnableAutobrightnessHandler const&)
    {
        return NullHandlerRegistration{};
    }

    repowerd::HandlerRegistration register_set_normal_brightness_value_handler(
        repowerd::SetNormalBrightnessValueHandler const&)
    {
        return NullHandlerRegistration{};
    }
};

struct NullDisplayPowerControl : repowerd::DisplayPowerControl
{
    void turn_on() override {}
    void turn_off() override {}
};

struct NullNotificationService : repowerd::NotificationService
{
    repowerd::HandlerRegistration register_all_notifications_done_handler(
        repowerd::AllNotificationsDoneHandler const&)
    {
        return NullHandlerRegistration{};
    }

    repowerd::HandlerRegistration register_notification_handler(
        repowerd::NotificationHandler const&)
    {
        return NullHandlerRegistration{};
    }
};

struct NullPowerButton : repowerd::PowerButton
{
    repowerd::HandlerRegistration register_power_button_handler(
        repowerd::PowerButtonHandler const&) override
    {
        return NullHandlerRegistration{};
    }
};

struct NullPowerButtonEventSink : repowerd::PowerButtonEventSink
{
    void notify_long_press() override {}
};

struct NullProximitySensor : repowerd::ProximitySensor
{
    repowerd::HandlerRegistration register_proximity_handler(
        repowerd::ProximityHandler const&) override
    {
        return NullHandlerRegistration{};
    }
    repowerd::ProximityState proximity_state() override { return {}; }
    void enable_proximity_events() override {}
    void disable_proximity_events() override {}
};

struct NullTimer : repowerd::Timer
{
    repowerd::HandlerRegistration register_alarm_handler(
        repowerd::AlarmHandler const&) override
    {
        return NullHandlerRegistration{};
    }
    repowerd::AlarmId schedule_alarm_in(std::chrono::milliseconds) override { return {}; }
    std::chrono::steady_clock::time_point now() { return {}; }
};

struct NullUserActivity : repowerd::UserActivity
{
    repowerd::HandlerRegistration register_user_activity_handler(
        repowerd::UserActivityHandler const&) override
    {
        return NullHandlerRegistration{};
    }
};

struct NullVoiceCallService : repowerd::VoiceCallService
{
    repowerd::HandlerRegistration register_active_call_handler(
        repowerd::ActiveCallHandler const&) override
    {
        return NullHandlerRegistration{};
    }

    repowerd::HandlerRegistration register_no_active_call_handler(
        repowerd::NoActiveCallHandler const&) override
    {
        return NullHandlerRegistration{};
    }
};

}

std::shared_ptr<repowerd::BrightnessControl>
repowerd::DefaultDaemonConfig::the_brightness_control()
{
    if (!brightness_control)
        brightness_control = std::make_shared<NullBrightnessControl>();
    return brightness_control;
}

std::shared_ptr<repowerd::ClientRequests>
repowerd::DefaultDaemonConfig::the_client_requests()
{
    if (!client_requests)
        client_requests = std::make_shared<NullClientRequests>();
    return client_requests;
}

std::shared_ptr<repowerd::DisplayPowerControl>
repowerd::DefaultDaemonConfig::the_display_power_control()
{
    if (!display_power_control)
        display_power_control = std::make_shared<NullDisplayPowerControl>();
    return display_power_control;
}

std::shared_ptr<repowerd::NotificationService>
repowerd::DefaultDaemonConfig::the_notification_service()
{
    if (!notification_service)
        notification_service = std::make_shared<NullNotificationService>();
    return notification_service;
}

std::shared_ptr<repowerd::PowerButton>
repowerd::DefaultDaemonConfig::the_power_button()
{
    if (!power_button)
        power_button = std::make_shared<NullPowerButton>();
    return power_button;
}

std::shared_ptr<repowerd::PowerButtonEventSink>
repowerd::DefaultDaemonConfig::the_power_button_event_sink()
{
    if (!power_button_event_sink)
        power_button_event_sink = std::make_shared<NullPowerButtonEventSink>();
    return power_button_event_sink;
}

std::shared_ptr<repowerd::ProximitySensor>
repowerd::DefaultDaemonConfig::the_proximity_sensor()
{
    if (!proximity_sensor)
        proximity_sensor = std::make_shared<NullProximitySensor>();
    return proximity_sensor;
}

std::shared_ptr<repowerd::StateMachine>
repowerd::DefaultDaemonConfig::the_state_machine()
{
    if (!state_machine)
        state_machine = std::make_shared<DefaultStateMachine>(*this);
    return state_machine;
}

std::shared_ptr<repowerd::Timer>
repowerd::DefaultDaemonConfig::the_timer()
{
    if (!timer)
        timer = std::make_shared<NullTimer>();
    return timer;
}

std::shared_ptr<repowerd::UserActivity>
repowerd::DefaultDaemonConfig::the_user_activity()
{
    if (!user_activity)
        user_activity = std::make_shared<NullUserActivity>();
    return user_activity;
}

std::shared_ptr<repowerd::VoiceCallService>
repowerd::DefaultDaemonConfig::the_voice_call_service()
{
    if (!voice_call_service)
        voice_call_service = std::make_shared<NullVoiceCallService>();
    return voice_call_service;
}

std::chrono::milliseconds
repowerd::DefaultDaemonConfig::power_button_long_press_timeout()
{
    return 2s;
}

std::chrono::milliseconds
repowerd::DefaultDaemonConfig::user_inactivity_normal_display_dim_duration()
{
    return 10s;
}

std::chrono::milliseconds
repowerd::DefaultDaemonConfig::user_inactivity_normal_display_off_timeout()
{
    return 60s;
}

std::chrono::milliseconds
repowerd::DefaultDaemonConfig::user_inactivity_reduced_display_off_timeout()
{
    return 15s;
}
