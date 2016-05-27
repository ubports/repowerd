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

#include "dbus_bus.h"
#include "dbus_client.h"
#include "fake_device_config.h"
#include "fake_log.h"
#include "fake_shared.h"
#include "fake_upower.h"
#include "spin_wait.h"
#include "wait_condition.h"

#include "src/adapters/upower_power_source.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>

namespace rt = repowerd::test;
using namespace testing;
using namespace std::chrono_literals;

namespace
{

struct AUPowerPowerSource : testing::Test
{
    AUPowerPowerSource()
    {
        registrations.push_back(
            upower_power_source.register_power_source_change_handler(
                [this]
                {
                    mock_handlers.power_source_change();
                }));

        registrations.push_back(
            upower_power_source.register_power_source_critical_handler(
                [this]
                {
                    mock_handlers.power_source_critical();
                }));

        fake_upower.add_device(device_path(0), unplugged_line_power);
        fake_upower.add_device(device_path(1), full_battery);

        upower_power_source.start_processing();
    }

    void wait_for_tracked_batteries(std::unordered_set<std::string> const& batteries)
    {
        auto const result = rt::spin_wait_for_condition_or_timeout(
            [this,&batteries] { return upower_power_source.tracked_batteries() == batteries; },
            default_timeout);
        if (!result)
            throw std::runtime_error("Timeout while waiting for tracked batteries");
    }

    std::string device_path(int i)
    {
        return "/org/freedesktop/UPower/device" + std::to_string(i);
    }

    struct MockHandlers
    {
        MOCK_METHOD0(power_source_change, void());
        MOCK_METHOD0(power_source_critical, void());
    };
    testing::NiceMock<MockHandlers> mock_handlers;

    rt::DBusBus bus;
    rt::FakeDeviceConfig fake_device_config;
    rt::FakeLog fake_log;
    repowerd::UPowerPowerSource upower_power_source{
        rt::fake_shared(fake_log),
        fake_device_config,
        bus.address()};
    rt::FakeUPower fake_upower{bus.address()};
    std::vector<repowerd::HandlerRegistration> registrations;

    rt::FakeUPower::DeviceInfo const plugged_line_power =
        rt::FakeUPower::DeviceInfo::for_plugged_line_power();
    rt::FakeUPower::DeviceInfo const unplugged_line_power =
        rt::FakeUPower::DeviceInfo::for_unplugged_line_power();
    rt::FakeUPower::DeviceInfo const full_battery =
        rt::FakeUPower::DeviceInfo::for_battery(rt::FakeUPower::DeviceState::fully_charged);
    rt::FakeUPower::DeviceInfo const discharging_battery =
        rt::FakeUPower::DeviceInfo::for_battery(rt::FakeUPower::DeviceState::discharging);
    rt::FakeUPower::DeviceInfo const charging_battery =
        rt::FakeUPower::DeviceInfo::for_battery(rt::FakeUPower::DeviceState::charging);

    std::chrono::seconds const default_timeout{3};
};

}

TEST_F(AUPowerPowerSource, notifies_of_change_from_full_to_discharging)
{
    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, power_source_change())
        .WillOnce(WakeUp(&request_processed));

    fake_upower.change_device(device_path(1), discharging_battery);

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());
}

TEST_F(AUPowerPowerSource, notifies_of_change_from_discharging_to_full)
{
    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, power_source_change())
        .WillOnce(Return())
        .WillOnce(WakeUp(&request_processed));

    fake_upower.change_device(device_path(1), discharging_battery);
    fake_upower.change_device(device_path(1), full_battery);

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());
}

TEST_F(AUPowerPowerSource, notifies_of_change_from_discharging_to_charging)
{
    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, power_source_change())
        .WillOnce(Return())
        .WillOnce(WakeUp(&request_processed));

    fake_upower.change_device(device_path(1), discharging_battery);
    fake_upower.change_device(device_path(1), charging_battery);

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());
}

TEST_F(AUPowerPowerSource,
       does_not_notify_of_change_from_full_to_charging_and_vice_versa)
{
    EXPECT_CALL(mock_handlers, power_source_change()).Times(0);

    fake_upower.change_device(device_path(1), charging_battery);
    fake_upower.change_device(device_path(1), full_battery);

    std::this_thread::sleep_for(100ms);
}

TEST_F(AUPowerPowerSource,
       notifies_of_change_for_battery_added_after_startup)
{
    fake_upower.add_device(device_path(2), full_battery);

    wait_for_tracked_batteries({device_path(1), device_path(2)});

    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, power_source_change())
        .WillOnce(WakeUp(&request_processed));

    fake_upower.change_device(device_path(2), discharging_battery);

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());
}

TEST_F(AUPowerPowerSource, notifies_of_change_for_removed_battery)
{
    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, power_source_change())
        .WillOnce(WakeUp(&request_processed));

    auto removed_battery = full_battery;
    removed_battery.is_present = false;

    fake_upower.change_device(device_path(1), removed_battery);

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());
}

TEST_F(AUPowerPowerSource, notifies_of_critical_state_for_low_battery_energy_when_unplugged)
{
    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, power_source_critical())
        .WillOnce(WakeUp(&request_processed));

    auto almost_empty_battery = discharging_battery;
    almost_empty_battery.percentage = 1.0;
    fake_upower.change_device(device_path(1), almost_empty_battery);

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());

    EXPECT_TRUE(fake_log.contains_line({"critical", "energy", "1.0%"}));
}

TEST_F(AUPowerPowerSource, does_not_notify_of_critical_state_for_low_battery_energy_when_plugged)
{
    EXPECT_CALL(mock_handlers, power_source_critical()).Times(0);

    auto removed_battery = full_battery;
    removed_battery.percentage = 1.0;
    fake_upower.change_device(device_path(0), plugged_line_power);
    fake_upower.change_device(device_path(1), removed_battery);

    std::this_thread::sleep_for(100ms);
}

TEST_F(AUPowerPowerSource, notifies_of_critical_state_for_high_battery_temperature_when_unplugged)
{
    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, power_source_critical())
        .WillOnce(WakeUp(&request_processed));

    auto exploding_battery = discharging_battery;
    // shutdown_battery_temperature is in celcius * 10
    exploding_battery.temperature = fake_device_config.shutdown_battery_temperature * 0.1;
    fake_upower.change_device(device_path(1), exploding_battery);

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());

    EXPECT_TRUE(fake_log.contains_line(
        {"critical",
         "temperature",
         std::to_string(exploding_battery.temperature).substr(0,4)}));
}

TEST_F(AUPowerPowerSource, notifies_of_critical_state_for_high_battery_temperature_when_plugged)
{
    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, power_source_critical())
        .WillOnce(WakeUp(&request_processed));

    auto exploding_battery = discharging_battery;
    // shutdown_battery_temperature is in celcius * 10
    exploding_battery.temperature = fake_device_config.shutdown_battery_temperature * 0.1;
    fake_upower.change_device(device_path(0), plugged_line_power);
    fake_upower.change_device(device_path(1), exploding_battery);

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());

    EXPECT_TRUE(fake_log.contains_line(
        {"critical",
         "temperature",
         std::to_string(exploding_battery.temperature).substr(0,4)}));
}

TEST_F(AUPowerPowerSource, does_not_notify_of_critical_state_for_removed_battery)
{
    EXPECT_CALL(mock_handlers, power_source_critical()).Times(0);

    auto removed_battery = full_battery;
    removed_battery.is_present = false;
    removed_battery.temperature = 100.0;
    removed_battery.percentage = 1.0;
    fake_upower.change_device(device_path(1), removed_battery);

    std::this_thread::sleep_for(100ms);
}
