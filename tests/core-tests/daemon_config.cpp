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
#include "src/core/default_state_machine_factory.h"

#include "fake_display_information.h"
#include "mock_brightness_control.h"
#include "fake_client_requests.h"
#include "fake_client_settings.h"
#include "mock_display_power_control.h"
#include "mock_display_power_event_sink.h"
#include "fake_lid.h"
#include "fake_log.h"
#include "mock_modem_power_control.h"
#include "fake_notification_service.h"
#include "mock_performance_booster.h"
#include "fake_power_button.h"
#include "mock_power_button_event_sink.h"
#include "fake_power_source.h"
#include "fake_proximity_sensor.h"
#include "fake_session_tracker.h"
#include "fake_state_machine_options.h"
#include "fake_system_power_control.h"
#include "fake_timer.h"
#include "fake_user_activity.h"
#include "fake_voice_call_service.h"

namespace rt = repowerd::test;
using testing::NiceMock;
using namespace std::chrono_literals;

std::shared_ptr<repowerd::DisplayInformation> rt::DaemonConfig::the_display_information()
{
    return the_fake_display_information();
}

std::shared_ptr<repowerd::BrightnessControl> rt::DaemonConfig::the_brightness_control()
{
    return the_mock_brightness_control();
}

std::shared_ptr<repowerd::ClientRequests> rt::DaemonConfig::the_client_requests()
{
    return the_fake_client_requests();
}

std::shared_ptr<repowerd::ClientSettings> rt::DaemonConfig::the_client_settings()
{
    return the_fake_client_settings();
}

std::shared_ptr<repowerd::DisplayPowerControl> rt::DaemonConfig::the_display_power_control()
{
    return the_mock_display_power_control();
}

std::shared_ptr<repowerd::DisplayPowerEventSink> rt::DaemonConfig::the_display_power_event_sink()
{
    return the_mock_display_power_event_sink();
}

std::shared_ptr<repowerd::Lid> rt::DaemonConfig::the_lid()
{
    return the_fake_lid();
}

std::shared_ptr<repowerd::Log> rt::DaemonConfig::the_log()
{
    return the_fake_log();
}

std::shared_ptr<repowerd::ModemPowerControl> rt::DaemonConfig::the_modem_power_control()
{
    return the_mock_modem_power_control();
}

std::shared_ptr<repowerd::NotificationService> rt::DaemonConfig::the_notification_service()
{
    return the_fake_notification_service();
}

std::shared_ptr<repowerd::PerformanceBooster> rt::DaemonConfig::the_performance_booster()
{
    return the_mock_performance_booster();
}

std::shared_ptr<repowerd::PowerButton> rt::DaemonConfig::the_power_button()
{
    return the_fake_power_button();
}

std::shared_ptr<repowerd::PowerButtonEventSink> rt::DaemonConfig::the_power_button_event_sink()
{
    return the_mock_power_button_event_sink();
}

std::shared_ptr<repowerd::PowerSource> rt::DaemonConfig::the_power_source()
{
    return the_fake_power_source();
}

std::shared_ptr<repowerd::ProximitySensor> rt::DaemonConfig::the_proximity_sensor()
{
    return the_fake_proximity_sensor();
}

std::shared_ptr<repowerd::SessionTracker> rt::DaemonConfig::the_session_tracker()
{
    return the_fake_session_tracker();
}

std::shared_ptr<repowerd::StateMachineFactory> rt::DaemonConfig::the_state_machine_factory()
{
    if (!state_machine_factory)
        state_machine_factory = std::make_shared<DefaultStateMachineFactory>(*this);
    return state_machine_factory;
}

std::shared_ptr<repowerd::StateMachineOptions> rt::DaemonConfig::the_state_machine_options()
{
    if (!state_machine_options)
        state_machine_options = std::make_shared<FakeStateMachineOptions>();
    return state_machine_options;
}

std::shared_ptr<repowerd::SystemPowerControl> rt::DaemonConfig::the_system_power_control()
{
    return the_fake_system_power_control();
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

std::shared_ptr<rt::FakeDisplayInformation> rt::DaemonConfig::the_fake_display_information()
{
    if (!fake_display_information)
        fake_display_information = std::make_shared<rt::FakeDisplayInformation>();

    return fake_display_information;
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

std::shared_ptr<rt::FakeClientSettings> rt::DaemonConfig::the_fake_client_settings()
{
    if (!fake_client_settings)
        fake_client_settings = std::make_shared<rt::FakeClientSettings>();

    return fake_client_settings;
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

std::shared_ptr<rt::FakeLid> rt::DaemonConfig::the_fake_lid()
{
    if (!fake_lid)
        fake_lid = std::make_shared<FakeLid>();

    return fake_lid;
}

std::shared_ptr<rt::FakeLog> rt::DaemonConfig::the_fake_log()
{
    if (!fake_log)
        fake_log = std::make_shared<FakeLog>();

    return fake_log;
}

std::shared_ptr<NiceMock<rt::MockModemPowerControl>>
rt::DaemonConfig::the_mock_modem_power_control()
{
    if (!mock_modem_power_control)
        mock_modem_power_control = std::make_shared<NiceMock<rt::MockModemPowerControl>>();

    return mock_modem_power_control;
}

std::shared_ptr<rt::FakeNotificationService> rt::DaemonConfig::the_fake_notification_service()
{
    if (!fake_notification_service)
        fake_notification_service = std::make_shared<rt::FakeNotificationService>();

    return fake_notification_service;
}

std::shared_ptr<NiceMock<rt::MockPerformanceBooster>>
rt::DaemonConfig::the_mock_performance_booster()
{
    if (!mock_performance_booster)
        mock_performance_booster = std::make_shared<NiceMock<rt::MockPerformanceBooster>>();

    return mock_performance_booster;
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

std::shared_ptr<rt::FakePowerSource> rt::DaemonConfig::the_fake_power_source()
{
    if (!fake_power_source)
        fake_power_source = std::make_shared<rt::FakePowerSource>();

    return fake_power_source;
}

std::shared_ptr<rt::FakeProximitySensor> rt::DaemonConfig::the_fake_proximity_sensor()
{
    if (!fake_proximity_sensor)
        fake_proximity_sensor = std::make_shared<rt::FakeProximitySensor>();

    return fake_proximity_sensor;
}

std::shared_ptr<rt::FakeSessionTracker> rt::DaemonConfig::the_fake_session_tracker()
{
    if (!fake_session_tracker)
        fake_session_tracker = std::make_shared<rt::FakeSessionTracker>();

    return fake_session_tracker;
}

std::shared_ptr<rt::FakeSystemPowerControl>
rt::DaemonConfig::the_fake_system_power_control()
{
    if (!fake_system_power_control)
        fake_system_power_control = std::make_shared<rt::FakeSystemPowerControl>();

    return fake_system_power_control;
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
