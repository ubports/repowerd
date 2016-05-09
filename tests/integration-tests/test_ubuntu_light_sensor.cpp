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

#include "src/adapters/ubuntu_light_sensor.h"

#include "temporary_environment_value.h"
#include "temporary_file.h"
#include "wait_condition.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>

#include <cstdlib>
#include <unistd.h>

namespace rt = repowerd::test;
using namespace testing;

namespace
{

struct AUbuntuLightSensor : testing::Test
{
    void set_up_sensor(std::string const& script)
    {
        rt::TemporaryFile command_file;
        rt::TemporaryEnvironmentValue backend{"UBUNTU_PLATFORM_API_BACKEND", "test"};
        rt::TemporaryEnvironmentValue test_file{"UBUNTU_PLATFORM_API_SENSOR_TEST", command_file.name().c_str()};
        command_file.write(script);

        sensor = std::make_unique<repowerd::UbuntuLightSensor>();
        registration = sensor->register_light_handler(
            [this](double light) { mock_handlers.light_handler(light); });
    }

    struct MockHandlers
    {
        MOCK_METHOD1(light_handler, void(double));
    };
    NiceMock<MockHandlers> mock_handlers;

    std::unique_ptr<repowerd::UbuntuLightSensor> sensor;
    repowerd::HandlerRegistration registration;

    std::chrono::seconds const default_timeout{3};
};

}

#define TEST_IN_SEPARATE_PROCESS(CODE) \
    EXPECT_EXIT({ \
        CODE \
        this->~AUbuntuLightSensor(); \
        exit(HasFailure() ? 1 : 0); \
        }, \
        ExitedWithCode(0), "")

TEST_F(AUbuntuLightSensor, reports_light_events)
{
    TEST_IN_SEPARATE_PROCESS({
        rt::WaitCondition handler_called;

        InSequence seq;
        EXPECT_CALL(mock_handlers, light_handler(10));
        EXPECT_CALL(mock_handlers, light_handler(30))
            .WillOnce(WakeUp(&handler_called));

        set_up_sensor(
            "create light 0 100 1\n"
            "500 light 10\n"
            "50 light 30\n");

        sensor->enable_light_events();

        handler_called.wait_for(default_timeout);
        EXPECT_TRUE(handler_called.woken());
    });
}
