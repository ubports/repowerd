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

#include "daemon_config.h"
#include "mock_brightness_control.h"
#include "mock_display_power_control.h"
#include "mock_display_power_event_sink.h"
#include "mock_power_button_event_sink.h"
#include "mock_shutdown_control.h"
#include "fake_client_requests.h"
#include "fake_log.h"
#include "fake_notification_service.h"
#include "fake_power_button.h"
#include "fake_power_source.h"
#include "fake_proximity_sensor.h"
#include "fake_timer.h"
#include "fake_user_activity.h"
#include "fake_voice_call_service.h"

#include "src/core/daemon.h"
#include "src/core/infinite_timeout.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace rt = repowerd::test;

rt::AcceptanceTest::AcceptanceTest()
    : power_button_long_press_timeout{
          config.power_button_long_press_timeout()},
      user_inactivity_normal_display_dim_duration{
          config.user_inactivity_normal_display_dim_duration()},
      user_inactivity_normal_display_dim_timeout{
          config.user_inactivity_normal_display_off_timeout() -
          config.user_inactivity_normal_display_dim_duration()},
      user_inactivity_normal_display_off_timeout{
          config.user_inactivity_normal_display_off_timeout()},
      user_inactivity_post_notification_display_off_timeout{
          config.user_inactivity_post_notification_display_off_timeout()},
      user_inactivity_reduced_display_off_timeout{
          config.user_inactivity_reduced_display_off_timeout()},
      infinite_timeout{repowerd::infinite_timeout}
{
    daemon_thread = std::thread{ [this] { daemon.run(); }};
    daemon.flush();
}

rt::AcceptanceTest::~AcceptanceTest()
{
    daemon.flush();
    daemon.stop();
    daemon_thread.join();
}

void rt::AcceptanceTest::expect_display_turns_off()
{
    EXPECT_CALL(*config.the_mock_display_power_control(), turn_off());
    EXPECT_CALL(*config.the_mock_brightness_control(), set_off_brightness());
}

void rt::AcceptanceTest::expect_display_turns_on()
{
    EXPECT_CALL(*config.the_mock_display_power_control(), turn_on());
    EXPECT_CALL(*config.the_mock_brightness_control(), set_normal_brightness());
}

void rt::AcceptanceTest::expect_display_dims()
{
    EXPECT_CALL(*config.the_mock_brightness_control(), set_dim_brightness());
}

void rt::AcceptanceTest::expect_display_brightens()
{
    EXPECT_CALL(*config.the_mock_brightness_control(), set_normal_brightness());
}

void rt::AcceptanceTest::expect_long_press_notification()
{
    EXPECT_CALL(*config.the_mock_power_button_event_sink(), notify_long_press());
}

void rt::AcceptanceTest::expect_no_display_brightness_change()
{
    EXPECT_CALL(*config.the_mock_brightness_control(), set_dim_brightness()).Times(0);
    EXPECT_CALL(*config.the_mock_brightness_control(), set_normal_brightness()).Times(0);
    EXPECT_CALL(*config.the_mock_brightness_control(), set_off_brightness()).Times(0);
}

void rt::AcceptanceTest::expect_no_display_power_change()
{
    EXPECT_CALL(*config.the_mock_display_power_control(), turn_on()).Times(0);
    EXPECT_CALL(*config.the_mock_display_power_control(), turn_off()).Times(0);
}

void rt::AcceptanceTest::expect_display_power_off_notification(
    DisplayPowerChangeReason reason)
{
    EXPECT_CALL(*config.the_mock_display_power_event_sink(),
                notify_display_power_off(reason));

}

void rt::AcceptanceTest::expect_display_power_on_notification(
    DisplayPowerChangeReason reason)
{
    EXPECT_CALL(*config.the_mock_display_power_event_sink(),
                notify_display_power_on(reason));
}

void rt::AcceptanceTest::expect_system_powers_off()
{
    EXPECT_CALL(*config.the_mock_shutdown_control(), power_off());
}

bool rt::AcceptanceTest::log_contains_line(std::vector<std::string> const& words)
{
    return config.the_fake_log()->contains_line(words);
}

void rt::AcceptanceTest::verify_expectations()
{
    daemon.flush();
    testing::Mock::VerifyAndClearExpectations(config.the_mock_brightness_control().get());
    testing::Mock::VerifyAndClearExpectations(config.the_mock_display_power_control().get());
    testing::Mock::VerifyAndClearExpectations(config.the_mock_power_button_event_sink().get());
}

void rt::AcceptanceTest::advance_time_by(std::chrono::milliseconds advance)
{
    daemon.flush();
    config.the_fake_timer()->advance_by(advance);
    daemon.flush();
}

void rt::AcceptanceTest::client_request_disable_inactivity_timeout()
{
    config.the_fake_client_requests()->emit_disable_inactivity_timeout();
    daemon.flush();
}

void rt::AcceptanceTest::client_request_enable_inactivity_timeout()
{
    config.the_fake_client_requests()->emit_enable_inactivity_timeout();
    daemon.flush();
}

void rt::AcceptanceTest::client_request_set_inactivity_timeout(
    std::chrono::milliseconds timeout)
{
    config.the_fake_client_requests()->emit_set_inactivity_timeout(timeout);
    daemon.flush();
}

void rt::AcceptanceTest::emit_no_notification()
{
    config.the_fake_notification_service()->emit_no_notification();
    daemon.flush();
}

void rt::AcceptanceTest::emit_notification()
{
    config.the_fake_notification_service()->emit_notification();
    daemon.flush();
}

void rt::AcceptanceTest::emit_power_source_change()
{
    config.the_fake_power_source()->emit_power_source_change();
    daemon.flush();
}

void rt::AcceptanceTest::emit_power_source_critical()
{
    config.the_fake_power_source()->emit_power_source_critical();
    daemon.flush();
}

void rt::AcceptanceTest::emit_proximity_state_far()
{
    config.the_fake_proximity_sensor()->emit_proximity_state(
        repowerd::ProximityState::far);
    daemon.flush();
}

void rt::AcceptanceTest::emit_proximity_state_far_if_enabled()
{
    config.the_fake_proximity_sensor()->emit_proximity_state_if_enabled(
        repowerd::ProximityState::far);
}

void rt::AcceptanceTest::emit_proximity_state_near()
{
    config.the_fake_proximity_sensor()->emit_proximity_state(
        repowerd::ProximityState::near);
    daemon.flush();
}

void rt::AcceptanceTest::emit_proximity_state_near_if_enabled()
{
    config.the_fake_proximity_sensor()->emit_proximity_state_if_enabled(
        repowerd::ProximityState::near);
}

void rt::AcceptanceTest::emit_active_call()
{
    config.the_fake_voice_call_service()->emit_active_call();
    daemon.flush();
}

void rt::AcceptanceTest::emit_no_active_call()
{
    config.the_fake_voice_call_service()->emit_no_active_call();
    daemon.flush();
}

void rt::AcceptanceTest::perform_user_activity_extending_power_state()
{
    config.the_fake_user_activity()->perform(
        repowerd::UserActivityType::extend_power_state);
    daemon.flush();
}

void rt::AcceptanceTest::perform_user_activity_changing_power_state()
{
    config.the_fake_user_activity()->perform(
        repowerd::UserActivityType::change_power_state);
    daemon.flush();
}

void rt::AcceptanceTest::press_power_button()
{
    config.the_fake_power_button()->press();
    daemon.flush();
}

void rt::AcceptanceTest::release_power_button()
{
    config.the_fake_power_button()->release();
    daemon.flush();
}

void rt::AcceptanceTest::set_proximity_state_far()
{
    config.the_fake_proximity_sensor()->set_proximity_state(
        repowerd::ProximityState::far);
}

void rt::AcceptanceTest::set_proximity_state_near()
{
    config.the_fake_proximity_sensor()->set_proximity_state(
        repowerd::ProximityState::near);
}

void rt::AcceptanceTest::turn_off_display()
{
    EXPECT_CALL(*config.the_mock_display_power_control(), turn_off());
    EXPECT_CALL(*config.the_mock_brightness_control(), set_off_brightness());
    press_power_button();
    release_power_button();
    daemon.flush();
    testing::Mock::VerifyAndClearExpectations(config.the_mock_display_power_control().get());
    testing::Mock::VerifyAndClearExpectations(config.the_mock_brightness_control().get());
}

void rt::AcceptanceTest::turn_on_display()
{
    EXPECT_CALL(*config.the_mock_display_power_control(), turn_on());
    EXPECT_CALL(*config.the_mock_brightness_control(), set_normal_brightness());
    press_power_button();
    release_power_button();
    daemon.flush();
    testing::Mock::VerifyAndClearExpectations(config.the_mock_display_power_control().get());
    testing::Mock::VerifyAndClearExpectations(config.the_mock_brightness_control().get());
}
