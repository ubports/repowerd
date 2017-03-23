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

#include "acceptance_test.h"
#include "run_daemon.h"

#include "daemon_config.h"
#include "mock_brightness_control.h"
#include "mock_display_power_control.h"
#include "mock_display_power_event_sink.h"
#include "mock_power_button_event_sink.h"
#include "fake_client_requests.h"
#include "fake_client_settings.h"
#include "fake_lid.h"
#include "fake_log.h"
#include "fake_notification_service.h"
#include "fake_power_button.h"
#include "fake_power_source.h"
#include "fake_proximity_sensor.h"
#include "fake_session_tracker.h"
#include "fake_system_power_control.h"
#include "fake_timer.h"
#include "fake_user_activity.h"
#include "fake_voice_call_service.h"

#include "src/core/daemon.h"
#include "src/core/infinite_timeout.h"
#include "src/core/state_machine_options.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace rt = repowerd::test;
using namespace testing;

rt::AcceptanceTest::AcceptanceTest()
    : AcceptanceTestBase{std::make_shared<rt::DaemonConfig>()}
{
    run_daemon();
}

rt::AcceptanceTestBase::AcceptanceTestBase(
    std::shared_ptr<DaemonConfig> const& daemon_config_ptr)
    : config_ptr{daemon_config_ptr},
      config{*daemon_config_ptr},
      notification_expiration_timeout{
          config.the_state_machine_options()->notification_expiration_timeout()},
      power_button_long_press_timeout{
          config.the_state_machine_options()->power_button_long_press_timeout()},
      user_inactivity_normal_display_dim_duration{
          config.the_state_machine_options()->user_inactivity_normal_display_dim_duration()},
      user_inactivity_normal_display_dim_timeout{
          config.the_state_machine_options()->user_inactivity_normal_display_off_timeout() -
          config.the_state_machine_options()->user_inactivity_normal_display_dim_duration()},
      user_inactivity_normal_display_off_timeout{
          config.the_state_machine_options()->user_inactivity_normal_display_off_timeout()},
      user_inactivity_normal_suspend_timeout{
          config.the_state_machine_options()->user_inactivity_normal_suspend_timeout()},
      user_inactivity_post_notification_display_off_timeout{
          config.the_state_machine_options()->user_inactivity_post_notification_display_off_timeout()},
      user_inactivity_reduced_display_off_timeout{
          config.the_state_machine_options()->user_inactivity_reduced_display_off_timeout()},
      infinite_timeout{repowerd::infinite_timeout},
      default_session_id{
          config.the_fake_session_tracker()->default_session()}
{
}

rt::AcceptanceTestBase::~AcceptanceTestBase()
{
    daemon.flush();
    daemon.stop();
    if (daemon_thread.joinable())
        daemon_thread.join();
}

void rt::AcceptanceTestBase::run_daemon()
{
    daemon_thread = rt::run_daemon(daemon);
}

void rt::AcceptanceTestBase::expect_autobrightness_disabled()
{
    EXPECT_CALL(*config.the_mock_brightness_control(), disable_autobrightness());
}

void rt::AcceptanceTestBase::expect_autobrightness_enabled()
{
    EXPECT_CALL(*config.the_mock_brightness_control(), enable_autobrightness());
}

void rt::AcceptanceTestBase::expect_display_turns_off()
{
    EXPECT_CALL(*config.the_mock_brightness_control(), set_off_brightness());
    EXPECT_CALL(*config.the_mock_display_power_control(), turn_off(DisplayPowerControlFilter::all));
}

void rt::AcceptanceTestBase::expect_display_turns_on()
{
    EXPECT_CALL(*config.the_mock_display_power_control(), turn_on(DisplayPowerControlFilter::all));
    EXPECT_CALL(*config.the_mock_brightness_control(), set_normal_brightness());
}

void rt::AcceptanceTestBase::expect_display_dims()
{
    EXPECT_CALL(*config.the_mock_brightness_control(), set_dim_brightness());
}

void rt::AcceptanceTestBase::expect_display_brightens()
{
    EXPECT_CALL(*config.the_mock_brightness_control(), set_normal_brightness());
}

void rt::AcceptanceTestBase::expect_internal_display_turns_off()
{
    EXPECT_CALL(*config.the_mock_display_power_control(),
                turn_off(DisplayPowerControlFilter::internal));
}

void rt::AcceptanceTestBase::expect_internal_display_turns_on()
{
    EXPECT_CALL(*config.the_mock_display_power_control(),
                turn_on(DisplayPowerControlFilter::internal));
}

void rt::AcceptanceTestBase::expect_external_display_turns_on()
{
    EXPECT_CALL(*config.the_mock_display_power_control(),
                turn_on(DisplayPowerControlFilter::external));
}

void rt::AcceptanceTestBase::expect_long_press_notification()
{
    EXPECT_CALL(*config.the_mock_power_button_event_sink(), notify_long_press());
}

void rt::AcceptanceTestBase::expect_no_autobrightness_change()
{
    EXPECT_CALL(*config.the_mock_brightness_control(), disable_autobrightness()).Times(0);
    EXPECT_CALL(*config.the_mock_brightness_control(), enable_autobrightness()).Times(0);
}

void rt::AcceptanceTestBase::expect_no_display_brightness_change()
{
    EXPECT_CALL(*config.the_mock_brightness_control(), set_dim_brightness()).Times(0);
    EXPECT_CALL(*config.the_mock_brightness_control(), set_normal_brightness()).Times(0);
    EXPECT_CALL(*config.the_mock_brightness_control(), set_off_brightness()).Times(0);
    EXPECT_CALL(*config.the_mock_brightness_control(), set_normal_brightness_value(_)).Times(0);
}

void rt::AcceptanceTestBase::expect_no_display_power_change()
{
    EXPECT_CALL(*config.the_mock_display_power_control(), turn_on(_)).Times(0);
    EXPECT_CALL(*config.the_mock_display_power_control(), turn_off(_)).Times(0);
}

void rt::AcceptanceTestBase::expect_no_long_press_notification()
{
    EXPECT_CALL(*config.the_mock_power_button_event_sink(), notify_long_press()).Times(0);
}

void rt::AcceptanceTestBase::expect_no_system_power_change()
{
    EXPECT_CALL(config.the_fake_system_power_control()->mock, power_off()).Times(0);
    EXPECT_CALL(config.the_fake_system_power_control()->mock, suspend()).Times(0);
    EXPECT_CALL(config.the_fake_system_power_control()->mock, suspend_if_allowed()).Times(0);
    EXPECT_CALL(config.the_fake_system_power_control()->mock, suspend_when_allowed(_)).Times(0);
}

void rt::AcceptanceTestBase::expect_normal_brightness_value_set_to(double value)
{
    EXPECT_CALL(*config.the_mock_brightness_control(), set_normal_brightness_value(value));
}

void rt::AcceptanceTestBase::expect_display_power_off_notification(
    DisplayPowerChangeReason reason)
{
    EXPECT_CALL(*config.the_mock_display_power_event_sink(),
                notify_display_power_off(reason));

}

void rt::AcceptanceTestBase::expect_display_power_on_notification(
    DisplayPowerChangeReason reason)
{
    EXPECT_CALL(*config.the_mock_display_power_event_sink(),
                notify_display_power_on(reason));
}

void rt::AcceptanceTestBase::expect_system_powers_off()
{
    EXPECT_CALL(config.the_fake_system_power_control()->mock, power_off());
}

void rt::AcceptanceTestBase::expect_system_suspends()
{
    EXPECT_CALL(config.the_fake_system_power_control()->mock, suspend());
}

void rt::AcceptanceTestBase::expect_system_suspends_when_allowed(
    std::string const& substr)
{
    EXPECT_CALL(config.the_fake_system_power_control()->mock,
                suspend_when_allowed(HasSubstr(substr)));
}

void rt::AcceptanceTestBase::expect_system_cancel_suspend_when_allowed(
    std::string const& substr)
{
    EXPECT_CALL(config.the_fake_system_power_control()->mock,
                cancel_suspend_when_allowed(HasSubstr(substr)));
}

bool rt::AcceptanceTestBase::log_contains_line(std::vector<std::string> const& words)
{
    return config.the_fake_log()->contains_line(words);
}

void rt::AcceptanceTestBase::verify_expectations()
{
    daemon.flush();
    testing::Mock::VerifyAndClearExpectations(config.the_mock_brightness_control().get());
    testing::Mock::VerifyAndClearExpectations(config.the_mock_display_power_control().get());
    testing::Mock::VerifyAndClearExpectations(config.the_mock_power_button_event_sink().get());
}

void rt::AcceptanceTestBase::advance_time_by(std::chrono::milliseconds advance)
{
    daemon.flush();
    config.the_fake_timer()->advance_by(advance);
    daemon.flush();
}

void rt::AcceptanceTestBase::client_request_disable_inactivity_timeout(
    std::string const& id, pid_t pid)
{
    config.the_fake_client_requests()->emit_disable_inactivity_timeout(id, pid);
    daemon.flush();
}

void rt::AcceptanceTestBase::client_request_enable_inactivity_timeout(
    std::string const& id, pid_t pid)
{
    config.the_fake_client_requests()->emit_enable_inactivity_timeout(id, pid);
    daemon.flush();
}

void rt::AcceptanceTestBase::client_request_set_inactivity_timeout(
    std::chrono::milliseconds timeout, pid_t pid)
{
    config.the_fake_client_requests()->emit_set_inactivity_timeout(timeout, pid);
    daemon.flush();
}

void rt::AcceptanceTestBase::client_request_disable_autobrightness(pid_t pid)
{
    config.the_fake_client_requests()->emit_disable_autobrightness(pid);
    daemon.flush();
}

void rt::AcceptanceTestBase::client_request_enable_autobrightness(pid_t pid)
{
    config.the_fake_client_requests()->emit_enable_autobrightness(pid);
    daemon.flush();
}

void rt::AcceptanceTestBase::client_request_set_normal_brightness_value(double value, pid_t pid)
{
    config.the_fake_client_requests()->emit_set_normal_brightness_value(value, pid);
    daemon.flush();
}

void rt::AcceptanceTestBase::client_request_allow_suspend(
    std::string const& id, pid_t pid)
{
    config.the_fake_client_requests()->emit_allow_suspend(id, pid);
    daemon.flush();
}

void rt::AcceptanceTestBase::client_request_disallow_suspend(
    std::string const& id, pid_t pid)
{
    config.the_fake_client_requests()->emit_disallow_suspend(id, pid);
    daemon.flush();
}

void rt::AcceptanceTestBase::client_setting_set_inactivity_behavior(
        PowerAction power_action,
        PowerSupply power_supply,
        std::chrono::milliseconds timeout,
        pid_t pid)
{
    config.the_fake_client_settings()->emit_set_inactivity_behavior(
        power_action, power_supply, timeout, pid);
    daemon.flush();
}

void rt::AcceptanceTestBase::client_setting_set_lid_behavior(
    PowerAction power_action,
    PowerSupply power_supply,
    pid_t pid)
{
    config.the_fake_client_settings()->emit_set_lid_behavior(
        power_action, power_supply, pid);
    daemon.flush();
}

void rt::AcceptanceTestBase::client_setting_set_critical_power_behavior(
    PowerAction power_action,
    pid_t pid)
{
    config.the_fake_client_settings()->emit_set_critical_power_behavior(
        power_action, pid);
    daemon.flush();
}

void rt::AcceptanceTestBase::close_lid()
{
    config.the_fake_lid()->close();
    daemon.flush();
}

void rt::AcceptanceTestBase::emit_notification(std::string const& id)
{
    config.the_fake_notification_service()->emit_notification(id);
    daemon.flush();
}

void rt::AcceptanceTestBase::emit_notification_done(std::string const& id)
{
    config.the_fake_notification_service()->emit_notification_done(id);
    daemon.flush();
}

void rt::AcceptanceTestBase::emit_notification()
{
    emit_notification("AcceptanceTestBaseId");
}

void rt::AcceptanceTestBase::emit_notification_done()
{
    emit_notification_done("AcceptanceTestBaseId");
}

void rt::AcceptanceTestBase::emit_power_source_change()
{
    config.the_fake_power_source()->emit_power_source_change();
    daemon.flush();
}

void rt::AcceptanceTestBase::emit_power_source_critical()
{
    config.the_fake_power_source()->emit_power_source_critical();
    daemon.flush();
}

void rt::AcceptanceTestBase::emit_proximity_state_far()
{
    config.the_fake_proximity_sensor()->emit_proximity_state(
        repowerd::ProximityState::far);
    daemon.flush();
}

void rt::AcceptanceTestBase::emit_proximity_state_far_if_enabled()
{
    config.the_fake_proximity_sensor()->emit_proximity_state_if_enabled(
        repowerd::ProximityState::far);
}

void rt::AcceptanceTestBase::emit_proximity_state_near()
{
    config.the_fake_proximity_sensor()->emit_proximity_state(
        repowerd::ProximityState::near);
    daemon.flush();
}

void rt::AcceptanceTestBase::emit_proximity_state_near_if_enabled()
{
    config.the_fake_proximity_sensor()->emit_proximity_state_if_enabled(
        repowerd::ProximityState::near);
}

void rt::AcceptanceTestBase::emit_active_call()
{
    config.the_fake_voice_call_service()->emit_active_call();
    daemon.flush();
}

void rt::AcceptanceTestBase::emit_no_active_call()
{
    config.the_fake_voice_call_service()->emit_no_active_call();
    daemon.flush();
}

void rt::AcceptanceTestBase::open_lid()
{
    config.the_fake_lid()->open();
    daemon.flush();
}

void rt::AcceptanceTestBase::perform_user_activity_extending_power_state()
{
    config.the_fake_user_activity()->perform(
        repowerd::UserActivityType::extend_power_state);
    daemon.flush();
}

void rt::AcceptanceTestBase::perform_user_activity_changing_power_state()
{
    config.the_fake_user_activity()->perform(
        repowerd::UserActivityType::change_power_state);
    daemon.flush();
}

void rt::AcceptanceTestBase::press_power_button()
{
    config.the_fake_power_button()->press();
    daemon.flush();
}

void rt::AcceptanceTestBase::release_power_button()
{
    config.the_fake_power_button()->release();
    daemon.flush();
}

void rt::AcceptanceTestBase::set_proximity_state_far()
{
    config.the_fake_proximity_sensor()->set_proximity_state(
        repowerd::ProximityState::far);
}

void rt::AcceptanceTestBase::set_proximity_state_near()
{
    config.the_fake_proximity_sensor()->set_proximity_state(
        repowerd::ProximityState::near);
}

void rt::AcceptanceTestBase::add_compatible_session(std::string const& session_id, pid_t pid)
{
    config.the_fake_session_tracker()->add_session(
        session_id, SessionType::RepowerdCompatible, pid);
    daemon.flush();
}

void rt::AcceptanceTestBase::add_incompatible_session(std::string const& session_id, pid_t pid)
{
    config.the_fake_session_tracker()->add_session(
        session_id, SessionType::RepowerdIncompatible, pid);
    daemon.flush();
}

void rt::AcceptanceTestBase::switch_to_session(std::string const& session_id)
{
    config.the_fake_session_tracker()->switch_to_session(session_id);
    daemon.flush();
}

void rt::AcceptanceTestBase::remove_session(std::string const& session_id)
{
    config.the_fake_session_tracker()->remove_session(session_id);
    daemon.flush();
}

void rt::AcceptanceTestBase::turn_off_display()
{
    EXPECT_CALL(*config.the_mock_display_power_control(), turn_off(DisplayPowerControlFilter::all));
    EXPECT_CALL(*config.the_mock_brightness_control(), set_off_brightness());
    press_power_button();
    release_power_button();
    daemon.flush();
    testing::Mock::VerifyAndClearExpectations(config.the_mock_display_power_control().get());
    testing::Mock::VerifyAndClearExpectations(config.the_mock_brightness_control().get());
}

void rt::AcceptanceTestBase::turn_on_display()
{
    EXPECT_CALL(*config.the_mock_display_power_control(), turn_on(DisplayPowerControlFilter::all));
    EXPECT_CALL(*config.the_mock_brightness_control(), set_normal_brightness());
    press_power_button();
    release_power_button();
    daemon.flush();
    testing::Mock::VerifyAndClearExpectations(config.the_mock_display_power_control().get());
    testing::Mock::VerifyAndClearExpectations(config.the_mock_brightness_control().get());
}

void rt::AcceptanceTestBase::use_battery_power()
{
    config.the_fake_power_source()->set_is_using_battery_power(true);
}

void rt::AcceptanceTestBase::use_line_power()
{
    config.the_fake_power_source()->set_is_using_battery_power(false);
}

bool rt::AcceptanceTestBase::are_proximity_events_enabled()
{
    return config.the_fake_proximity_sensor()->are_proximity_events_enabled();
}

bool rt::AcceptanceTestBase::are_default_system_handlers_allowed()
{
    return config.the_fake_system_power_control()->are_default_system_handlers_allowed();
}
