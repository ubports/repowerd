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

#include "src/adapters/ubuntu_proximity_sensor.h"
#include "src/adapters/device_quirks.h"

#include "fake_device_quirks.h"
#include "fake_log.h"
#include "fake_shared.h"
#include "temporary_environment_value.h"
#include "temporary_file.h"
#include "test_in_separate_process.h"
#include "wait_condition.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>

namespace rt = repowerd::test;
using namespace testing;

namespace
{

struct AUbuntuProximitySensor : testing::Test
{
    void set_up_sensor(std::string const& script)
    {
        rt::TemporaryFile command_file;
        rt::TemporaryEnvironmentValue backend{"UBUNTU_PLATFORM_API_BACKEND", "test"};
        rt::TemporaryEnvironmentValue test_file{"UBUNTU_PLATFORM_API_SENSOR_TEST", command_file.name().c_str()};
        command_file.write(script);

        sensor = std::make_unique<repowerd::UbuntuProximitySensor>(
            rt::fake_shared(fake_log), fake_device_quirks);
        registration = sensor->register_proximity_handler(
            [this](repowerd::ProximityState state) { mock_handlers.proximity_handler(state); });
    }

    struct MockHandlers
    {
        MOCK_METHOD1(proximity_handler, void(repowerd::ProximityState));
    };
    NiceMock<MockHandlers> mock_handlers;

    rt::FakeLog fake_log;
    rt::FakeDeviceQuirks fake_device_quirks;
    std::unique_ptr<repowerd::UbuntuProximitySensor> sensor;
    repowerd::HandlerRegistration registration;

    std::chrono::seconds const default_timeout{3};
};

}

TEST_F(AUbuntuProximitySensor, reports_far_event)
{
    TEST_IN_SEPARATE_PROCESS({
        rt::WaitCondition handler_called;

        EXPECT_CALL(mock_handlers, proximity_handler(repowerd::ProximityState::far))
            .WillOnce(WakeUp(&handler_called));

        set_up_sensor(
            "create proximity\n"
            "500 proximity far\n");

        sensor->enable_proximity_events();

        handler_called.wait_for(default_timeout);
        EXPECT_TRUE(handler_called.woken());

        EXPECT_TRUE(fake_log.contains_line({"proximity_event", "far"}));
    });
}


TEST_F(AUbuntuProximitySensor, reports_near_event)
{
    TEST_IN_SEPARATE_PROCESS({
        rt::WaitCondition handler_called;

        EXPECT_CALL(mock_handlers, proximity_handler(repowerd::ProximityState::near))
            .WillOnce(WakeUp(&handler_called));

        set_up_sensor(
            "create proximity\n"
            "500 proximity near\n");

        sensor->enable_proximity_events();

        handler_called.wait_for(default_timeout);
        EXPECT_TRUE(handler_called.woken());

        EXPECT_TRUE(fake_log.contains_line({"proximity_event", "near"}));
    });
}


TEST_F(AUbuntuProximitySensor, reports_synthetic_event_far_if_no_initial_event_arrives)
{
    TEST_IN_SEPARATE_PROCESS({
        rt::WaitCondition handler_called;

        EXPECT_CALL(mock_handlers, proximity_handler(repowerd::ProximityState::far))
            .WillOnce(WakeUp(&handler_called));

        set_up_sensor(
            "create proximity\n");

        sensor->enable_proximity_events();

        handler_called.wait_for(default_timeout);
        EXPECT_TRUE(handler_called.woken());

        EXPECT_TRUE(fake_log.contains_line({"schedule", "synthetic", "far"}));
        EXPECT_TRUE(fake_log.contains_line({"emitting", "synthetic", "far"}));
        EXPECT_TRUE(fake_log.contains_line({"proximity_event", "far"}));
    });
}

TEST_F(AUbuntuProximitySensor, reports_synthetic_event_near_if_no_initial_event_arrives)
{
    TEST_IN_SEPARATE_PROCESS({
        fake_device_quirks.set_synthetic_initial_event_type_near();

        rt::WaitCondition handler_called;

        EXPECT_CALL(mock_handlers, proximity_handler(repowerd::ProximityState::near))
            .WillOnce(WakeUp(&handler_called));

        set_up_sensor(
            "create proximity\n");

        sensor->enable_proximity_events();

        handler_called.wait_for(default_timeout);
        EXPECT_TRUE(handler_called.woken());

        EXPECT_TRUE(fake_log.contains_line({"schedule", "synthetic", "near"}));
        EXPECT_TRUE(fake_log.contains_line({"emitting", "synthetic", "near"}));
        EXPECT_TRUE(fake_log.contains_line({"proximity_event", "near"}));
    });
}

TEST_F(AUbuntuProximitySensor, reports_current_state)
{
    TEST_IN_SEPARATE_PROCESS({
        set_up_sensor(
            "create proximity\n"
            "500 proximity near\n");
        sensor->enable_proximity_events();

        std::this_thread::sleep_for(std::chrono::milliseconds{750});
        EXPECT_THAT(sensor->proximity_state(), Eq(repowerd::ProximityState::near));

        EXPECT_TRUE(fake_log.contains_line({"proximity_state", "near"}));
    });
}

TEST_F(AUbuntuProximitySensor, waits_for_first_event_to_report_current_state)
{
    TEST_IN_SEPARATE_PROCESS({
        set_up_sensor(
            "create proximity\n"
            "500 proximity near\n");

        EXPECT_THAT(sensor->proximity_state(), Eq(repowerd::ProximityState::near));

        EXPECT_TRUE(fake_log.contains_line({"proximity_state", "near"}));
    });
}

TEST_F(AUbuntuProximitySensor, reports_current_synthetic_state_far_if_no_event_arrives_soon)
{
    TEST_IN_SEPARATE_PROCESS({
        set_up_sensor(
            "create proximity\n");

        EXPECT_THAT(sensor->proximity_state(), Eq(repowerd::ProximityState::far));

        EXPECT_TRUE(fake_log.contains_line({"proximity_state", "far"}));
    });
}

TEST_F(AUbuntuProximitySensor, reports_current_synthetic_state_near_if_no_event_arrives_soon)
{
    TEST_IN_SEPARATE_PROCESS({
        fake_device_quirks.set_synthetic_initial_event_type_near();

        set_up_sensor(
            "create proximity\n");

        EXPECT_THAT(sensor->proximity_state(), Eq(repowerd::ProximityState::near));

        EXPECT_TRUE(fake_log.contains_line({"proximity_state", "near"}));
    });
}

TEST_F(AUbuntuProximitySensor, logs_enable_and_disable_proximity_events)
{
    TEST_IN_SEPARATE_PROCESS({
        set_up_sensor(
            "create proximity\n");

        sensor->enable_proximity_events();
        EXPECT_TRUE(fake_log.contains_line({"enable", "proximity_events"}));
        EXPECT_FALSE(fake_log.contains_line({"disable", "proximity_events"}));

        sensor->disable_proximity_events();
        EXPECT_TRUE(fake_log.contains_line({"disable", "proximity_events"}));
    });
}

TEST_F(AUbuntuProximitySensor, ignores_stray_proximity_events_when_disabled)
{
    TEST_IN_SEPARATE_PROCESS({
        EXPECT_CALL(mock_handlers, proximity_handler(_)).Times(0);

        set_up_sensor(
            "create proximity\n");

        sensor->emit_proximity_event(repowerd::ProximityState::near);
        EXPECT_TRUE(fake_log.contains_line({"handle_proximity_event", "near", "ignoring"}));

        EXPECT_THAT(sensor->proximity_state(), Eq(repowerd::ProximityState::far));
        std::this_thread::sleep_for(std::chrono::milliseconds{1100});
    });
}
