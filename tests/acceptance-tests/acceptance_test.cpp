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
#include "mock_power_button_event_sink.h"
#include "fake_client_requests.h"
#include "fake_notification_service.h"
#include "fake_power_button.h"
#include "fake_proximity_sensor.h"
#include "fake_timer.h"
#include "fake_user_activity.h"

#include "src/daemon.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace rt = repowerd::test;

rt::AcceptanceTest::AcceptanceTest()
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
}

void rt::AcceptanceTest::client_request_enable_inactivity_timeout()
{
    config.the_fake_client_requests()->emit_enable_inactivity_timeout();
}

void rt::AcceptanceTest::emit_all_notifications_done()
{
    config.the_fake_notification_service()->emit_all_notifications_done();
}

void rt::AcceptanceTest::emit_notification()
{
    config.the_fake_notification_service()->emit_notification();
}

void rt::AcceptanceTest::emit_proximity_state_far()
{
    config.the_fake_proximity_sensor()->emit_proximity_state(
        repowerd::ProximityState::far);
}

void rt::AcceptanceTest::emit_proximity_state_near()
{
    config.the_fake_proximity_sensor()->emit_proximity_state(
        repowerd::ProximityState::near);
}

void rt::AcceptanceTest::perform_user_activity_extending_power_state()
{
    config.the_fake_user_activity()->perform(
        repowerd::UserActivityType::extend_power_state);
}

void rt::AcceptanceTest::perform_user_activity_changing_power_state()
{
    config.the_fake_user_activity()->perform(
        repowerd::UserActivityType::change_power_state);
}

void rt::AcceptanceTest::press_power_button()
{
    config.the_fake_power_button()->press();
}

void rt::AcceptanceTest::release_power_button()
{
    config.the_fake_power_button()->release();
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
