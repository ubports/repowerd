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

class TemporaryEnvironmentValue
{
public:
    TemporaryEnvironmentValue(char const* name, char const* value)
        : name{name},
          has_old_value{getenv(name) != nullptr},
          old_value{has_old_value ? getenv(name) : ""}
    {
        if (value)
            setenv(name, value, overwrite);
        else
            unsetenv(name);
    }

    ~TemporaryEnvironmentValue()
    {
        if (has_old_value)
            setenv(name.c_str(), old_value.c_str(), overwrite);
        else
            unsetenv(name.c_str());
    }

private:
    static int constexpr overwrite = 1;
    std::string const name;
    bool const has_old_value;
    std::string const old_value;
};

class TemporaryFile
{
public:
    TemporaryFile()
    {
        char name_template[] = "/tmp/repowerd-test-XXXXXX";
        fd = mkstemp(name_template);
        if (fd == -1)
            throw std::runtime_error("Failed to create temporary file");
        filename = name_template;
    }

    ~TemporaryFile()
    {
        close(fd);
        unlink(filename.c_str());
    }

    std::string name()
    {
        return filename;
    }

    void write(std::string data)
    {
        ::write(fd, data.c_str(), data.size());
        fsync(fd);
    }

private:
    int fd;
    std::string filename;
};

struct StubDeviceQuirks : repowerd::DeviceQuirks
{
    std::chrono::milliseconds synthentic_initial_far_event_delay() const override
    {
        // Set a high delay to account for valgrind slowness
        return std::chrono::milliseconds{1000};
    }
};

struct AUbuntuProximitySensor : testing::Test
{
    void set_up_sensor(std::string const& script)
    {
        TemporaryFile command_file;
        TemporaryEnvironmentValue backend{"UBUNTU_PLATFORM_API_BACKEND", "test"};
        TemporaryEnvironmentValue test_file{"UBUNTU_PLATFORM_API_SENSOR_TEST", command_file.name().c_str()};
        command_file.write(script);

        sensor = std::make_unique<repowerd::UbuntuProximitySensor>(StubDeviceQuirks());
        registration = sensor->register_proximity_handler(
            [this](repowerd::ProximityState state) {
            std::cerr << "Handler called " << (int)state << std::endl;
            mock_handlers.proximity_handler(state); });
    }

    struct MockHandlers
    {
        MOCK_METHOD1(proximity_handler, void(repowerd::ProximityState));
    };
    NiceMock<MockHandlers> mock_handlers;


    std::unique_ptr<repowerd::UbuntuProximitySensor> sensor;
    repowerd::HandlerRegistration registration;

    std::chrono::seconds const default_timeout{3};
};

}

#define TEST_IN_SEPARATE_PROCESS(CODE) \
    EXPECT_EXIT({ \
        CODE \
        this->~AUbuntuProximitySensor(); \
        exit(HasFailure() ? 1 : 0); \
        }, \
        ExitedWithCode(0), "")

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
    });
}


TEST_F(AUbuntuProximitySensor, reports_synthetic_far_event_if_no_initial_event_arrives)
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
    });
}

TEST_F(AUbuntuProximitySensor, waits_for_first_event_to_report_current_state)
{
    TEST_IN_SEPARATE_PROCESS({
        set_up_sensor(
            "create proximity\n"
            "500 proximity near\n");

        EXPECT_THAT(sensor->proximity_state(), Eq(repowerd::ProximityState::near));
    });
}

TEST_F(AUbuntuProximitySensor, reports_current_state_far_if_no_event_arrives_soon)
{
    TEST_IN_SEPARATE_PROCESS({
        set_up_sensor(
            "create proximity\n");

        EXPECT_THAT(sensor->proximity_state(), Eq(repowerd::ProximityState::far));
    });
}
