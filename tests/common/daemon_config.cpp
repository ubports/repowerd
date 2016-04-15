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

#include "daemon_config.h"
#include "src/core/default_state_machine.h"

#include "mock_brightness_control.h"
#include "fake_client_requests.h"
#include "mock_display_power_control.h"
#include "mock_display_power_event_sink.h"
#include "fake_notification_service.h"
#include "fake_power_button.h"
#include "mock_power_button_event_sink.h"
#include "fake_proximity_sensor.h"
#include "fake_timer.h"
#include "fake_user_activity.h"
#include "fake_voice_call_service.h"

namespace rt = repowerd::test;
using testing::NiceMock;
using namespace std::chrono_literals;

std::shared_ptr<repowerd::BrightnessControl> rt::DaemonConfig::the_brightness_control()
{
    return the_mock_brightness_control();
}

std::shared_ptr<repowerd::ClientRequests> rt::DaemonConfig::the_client_requests()
{
    return the_fake_client_requests();
}

std::shared_ptr<repowerd::DisplayPowerControl> rt::DaemonConfig::the_display_power_control()
{
    return the_mock_display_power_control();
}

std::shared_ptr<repowerd::DisplayPowerEventSink> rt::DaemonConfig::the_display_power_event_sink()
{
    return the_mock_display_power_event_sink();
}

std::shared_ptr<repowerd::NotificationService> rt::DaemonConfig::the_notification_service()
{
    return the_fake_notification_service();
}

std::shared_ptr<repowerd::PowerButton> rt::DaemonConfig::the_power_button()
{
    return the_fake_power_button();
}

std::shared_ptr<repowerd::PowerButtonEventSink> rt::DaemonConfig::the_power_button_event_sink()
{
    return the_mock_power_button_event_sink();
}

std::shared_ptr<repowerd::ProximitySensor> rt::DaemonConfig::the_proximity_sensor()
{
    return the_fake_proximity_sensor();
}

std::shared_ptr<repowerd::StateMachine> rt::DaemonConfig::the_state_machine()
{
    if (!state_machine)
        state_machine = std::make_shared<DefaultStateMachine>(*this);
    return state_machine;
}

std::shared_ptr<repowerd::Timer> rt::DaemonConfig::the_timer()
{
    return the_fake_timer();
}

std::shared_ptr<repowerd::UserActivity> rt::DaemonConfig::the_user_activity()
{
    return the_fake_user_activity();
}

std::shared_ptr<repowerd::VoiceCallService> rt::DaemonConfig::the_voice_call_service()
{
    return the_fake_voice_call_service();
}

std::chrono::milliseconds
rt::DaemonConfig::power_button_long_press_timeout()
{
    return 2s;
}

std::chrono::milliseconds
rt::DaemonConfig::user_inactivity_normal_display_dim_duration()
{
    return 10s;
}

std::chrono::milliseconds
rt::DaemonConfig::user_inactivity_normal_display_off_timeout()
{
    return 60s;
}

std::chrono::milliseconds
rt::DaemonConfig::user_inactivity_reduced_display_off_timeout()
{
    return 15s;
}

bool rt::DaemonConfig::turn_on_display_at_startup()
{
    return false;
}

std::shared_ptr<NiceMock<rt::MockBrightnessControl>>
rt::DaemonConfig::the_mock_brightness_control()
{
    if (!mock_brightness_control)
        mock_brightness_control = std::make_shared<NiceMock<rt::MockBrightnessControl>>();

    return mock_brightness_control;
}

std::shared_ptr<rt::FakeClientRequests> rt::DaemonConfig::the_fake_client_requests()
{
    if (!fake_client_requests)
        fake_client_requests = std::make_shared<rt::FakeClientRequests>();

    return fake_client_requests;
}

std::shared_ptr<NiceMock<rt::MockDisplayPowerControl>>
rt::DaemonConfig::the_mock_display_power_control()
{
    if (!mock_display_power_control)
        mock_display_power_control = std::make_shared<NiceMock<rt::MockDisplayPowerControl>>();

    return mock_display_power_control;
}

std::shared_ptr<NiceMock<rt::MockDisplayPowerEventSink>>
rt::DaemonConfig::the_mock_display_power_event_sink()
{
    if (!mock_display_power_event_sink)
        mock_display_power_event_sink = std::make_shared<NiceMock<rt::MockDisplayPowerEventSink>>();

    return mock_display_power_event_sink;
}

std::shared_ptr<rt::FakeNotificationService> rt::DaemonConfig::the_fake_notification_service()
{
    if (!fake_notification_service)
        fake_notification_service = std::make_shared<rt::FakeNotificationService>();

    return fake_notification_service;
}

std::shared_ptr<rt::FakePowerButton> rt::DaemonConfig::the_fake_power_button()
{
    if (!fake_power_button)
        fake_power_button = std::make_shared<rt::FakePowerButton>();

    return fake_power_button;
}

std::shared_ptr<NiceMock<rt::MockPowerButtonEventSink>>
rt::DaemonConfig::the_mock_power_button_event_sink()
{
    if (!mock_power_button_event_sink)
        mock_power_button_event_sink = std::make_shared<NiceMock<rt::MockPowerButtonEventSink>>();

    return mock_power_button_event_sink;
}

std::shared_ptr<rt::FakeProximitySensor> rt::DaemonConfig::the_fake_proximity_sensor()
{
    if (!fake_proximity_sensor)
        fake_proximity_sensor = std::make_shared<rt::FakeProximitySensor>();

    return fake_proximity_sensor;
}

std::shared_ptr<rt::FakeTimer> rt::DaemonConfig::the_fake_timer()
{
    if (!fake_timer)
        fake_timer = std::make_shared<rt::FakeTimer>();

    return fake_timer;
}

std::shared_ptr<rt::FakeUserActivity> rt::DaemonConfig::the_fake_user_activity()
{
    if (!fake_user_activity)
        fake_user_activity = std::make_shared<rt::FakeUserActivity>();

    return fake_user_activity;
}

std::shared_ptr<rt::FakeVoiceCallService> rt::DaemonConfig::the_fake_voice_call_service()
{
    if (!fake_voice_call_service)
        fake_voice_call_service = std::make_shared<rt::FakeVoiceCallService>();

    return fake_voice_call_service;
}
