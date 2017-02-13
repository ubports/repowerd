/*
 * Copyright © 2017 Canonical Ltd.
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

#include "src/adapters/repowerd_service.h"
#include "src/core/infinite_timeout.h"

#include "dbus_bus.h"
#include "fake_log.h"
#include "fake_shared.h"
#include "repowerd_dbus_client.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>

using namespace testing;
using namespace std::chrono_literals;

namespace rt = repowerd::test;

namespace
{

template <typename T> struct PowerArg
{
    T value;
    std::string str;
};

struct ARepowerdService : testing::Test
{
    ARepowerdService()
    {
        registrations.push_back(
            service.register_set_inactivity_behavior_handler(
                [this] (repowerd::PowerAction power_action,
                        repowerd::PowerSupply power_supply,
                        std::chrono::milliseconds timeout,
                        pid_t pid)
                {
                    mock_handlers.set_inactivity_behavior(
                        power_action, power_supply, timeout, pid);
                }));

        registrations.push_back(
            service.register_set_lid_behavior_handler(
                [this] (repowerd::PowerAction power_action,
                        repowerd::PowerSupply power_supply,
                        pid_t pid)
                {
                    mock_handlers.set_lid_behavior(
                        power_action, power_supply, pid);
                }));

        registrations.push_back(
            service.register_set_critical_power_behavior_handler(
                [this] (repowerd::PowerAction power_action,
                        pid_t pid)
                {
                    mock_handlers.set_critical_power_behavior(
                        power_action, pid);
                }));

        service.start_processing();
    }

    struct MockHandlers
    {
        MOCK_METHOD4(set_inactivity_behavior,
                     void(repowerd::PowerAction power_action,
                          repowerd::PowerSupply power_supply,
                          std::chrono::milliseconds timeout,
                          pid_t pid));
        MOCK_METHOD3(set_lid_behavior,
                     void(repowerd::PowerAction power_action,
                          repowerd::PowerSupply power_supply,
                          pid_t pid));
        MOCK_METHOD2(set_critical_power_behavior,
                     void(repowerd::PowerAction power_action,
                          pid_t pid));
    };
    testing::NiceMock<MockHandlers> mock_handlers;

    rt::DBusBus bus;
    rt::FakeLog fake_log;
    repowerd::RepowerdService service{
        rt::fake_shared(fake_log),
        bus.address()};
    rt::RepowerdDBusClient client{bus.address()};
    std::vector<repowerd::HandlerRegistration> registrations;

    PowerArg<repowerd::PowerSupply> power_supply_args[2]{
        { repowerd::PowerSupply::battery, "battery" },
        { repowerd::PowerSupply::line_power, "line-power" }};

    std::chrono::milliseconds const timeout{30000};
    int32_t const timeout_sec =
        std::chrono::duration_cast<std::chrono::seconds>(timeout).count();
};

}

TEST_F(ARepowerdService, replies_to_introspection_request)
{
    auto reply = client.request_introspection();
    EXPECT_THAT(reply.get(), StrNe(""));
}

TEST_F(ARepowerdService,
       forwards_set_inactivity_behavior_request_with_supported_actions)
{
    PowerArg<repowerd::PowerAction> power_action_args[] = {
        { repowerd::PowerAction::display_off, "display-off" },
        { repowerd::PowerAction::suspend, "suspend" }};

    for (auto const& action_arg : power_action_args)
    {
        for (auto const& supply_arg : power_supply_args)
        {
            EXPECT_CALL(mock_handlers,
                        set_inactivity_behavior(
                            action_arg.value,
                            supply_arg.value,
                            timeout,
                            _));

            client.request_set_inactivity_behavior(
                action_arg.str,
                supply_arg.str,
                timeout_sec);

            Mock::VerifyAndClearExpectations(&mock_handlers);
        }
    }
}

TEST_F(ARepowerdService,
       forwards_set_inactivity_behavior_request_with_infinite_timeout_for_non_positive_request_timeout)
{
    for (auto const request_timeout_sec : {-1, 0})
    {
        EXPECT_CALL(mock_handlers,
                    set_inactivity_behavior(
                        repowerd::PowerAction::display_off,
                        repowerd::PowerSupply::battery,
                        repowerd::infinite_timeout,
                        _));

        client.request_set_inactivity_behavior(
            "display-off",
            "battery",
            request_timeout_sec);

        Mock::VerifyAndClearExpectations(&mock_handlers);
    }
}

TEST_F(ARepowerdService,
       does_not_forward_set_inactivity_behavior_request_with_unsupported_actions)
{
    PowerArg<repowerd::PowerAction> power_action_args[] = {
        { repowerd::PowerAction::none, "invalid" },
        { repowerd::PowerAction::none, "none" },
        { repowerd::PowerAction::power_off, "power-off" }};

    for (auto const& action_arg : power_action_args)
    {
        for (auto const& supply_arg : power_supply_args)
        {
            EXPECT_CALL(mock_handlers,
                        set_inactivity_behavior(_, _, _, _)).Times(0);

            try
            {
                client.request_set_inactivity_behavior(
                    action_arg.str,
                    supply_arg.str,
                    timeout_sec).get();
            }
            catch (...)
            {
            }

            Mock::VerifyAndClearExpectations(&mock_handlers);
        }
    }
}

TEST_F(ARepowerdService, logs_set_inactivity_behavior_request)
{
    PowerArg<repowerd::PowerAction> power_action_args[] = {
        { repowerd::PowerAction::none, "invalid" },
        { repowerd::PowerAction::none, "none" },
        { repowerd::PowerAction::display_off, "display-off" },
        { repowerd::PowerAction::suspend, "suspend" },
        { repowerd::PowerAction::power_off, "power-off" }};

    for (auto const& action_arg : power_action_args)
    {
        for (auto const& supply_arg : power_supply_args)
        {
            try
            {
                client.request_set_inactivity_behavior(
                    action_arg.str,
                    supply_arg.str,
                    timeout_sec).get();
            }
            catch (...)
            {
            }

            EXPECT_TRUE(
                fake_log.contains_line(
                    {"SetInactivityBehavior",
                     action_arg.str,
                     supply_arg.str,
                     std::to_string(timeout_sec)}));
        }
    }
}

TEST_F(ARepowerdService,
       forwards_set_lid_behavior_request_with_supported_actions)
{
    PowerArg<repowerd::PowerAction> power_action_args[] = {
        { repowerd::PowerAction::none, "none" },
        { repowerd::PowerAction::suspend, "suspend" }};

    for (auto const& action_arg : power_action_args)
    {
        for (auto const& supply_arg : power_supply_args)
        {
            EXPECT_CALL(mock_handlers,
                        set_lid_behavior(action_arg.value, supply_arg.value, _));

            client.request_set_lid_behavior(action_arg.str, supply_arg.str);

            Mock::VerifyAndClearExpectations(&mock_handlers);
        }
    }
}

TEST_F(ARepowerdService,
       does_not_forward_set_lid_behavior_request_with_unsupported_actions)
{
    PowerArg<repowerd::PowerAction> power_action_args[] = {
        { repowerd::PowerAction::none, "invalid" },
        { repowerd::PowerAction::none, "display-off" },
        { repowerd::PowerAction::power_off, "power-off" }};

    for (auto const& action_arg : power_action_args)
    {
        for (auto const& supply_arg : power_supply_args)
        {
            EXPECT_CALL(mock_handlers, set_lid_behavior(_, _, _)).Times(0);

            try
            {
                client.request_set_lid_behavior(action_arg.str, supply_arg.str).get();
            }
            catch (...)
            {
            }

            Mock::VerifyAndClearExpectations(&mock_handlers);
        }
    }
}

TEST_F(ARepowerdService, logs_set_lid_behavior_request)
{
    PowerArg<repowerd::PowerAction> power_action_args[] = {
        { repowerd::PowerAction::none, "invalid" },
        { repowerd::PowerAction::none, "none" },
        { repowerd::PowerAction::display_off, "display-off" },
        { repowerd::PowerAction::suspend, "suspend" },
        { repowerd::PowerAction::power_off, "power-off" }};

    for (auto const& action_arg : power_action_args)
    {
        for (auto const& supply_arg : power_supply_args)
        {
            try
            {
                client.request_set_lid_behavior(action_arg.str, supply_arg.str).get();
            }
            catch (...)
            {
            }

            EXPECT_TRUE(
                fake_log.contains_line(
                    {"SetLidBehavior",
                     action_arg.str,
                     supply_arg.str}));
        }
    }
}

TEST_F(ARepowerdService,
       forwards_set_critical_power_behavior_request_with_supported_actions)
{
    PowerArg<repowerd::PowerAction> power_action_args[] = {
        { repowerd::PowerAction::suspend, "suspend" },
        { repowerd::PowerAction::power_off, "power-off" }};

    for (auto const& action_arg : power_action_args)
    {
        EXPECT_CALL(mock_handlers,
                    set_critical_power_behavior(action_arg.value, _));

        client.request_set_critical_power_behavior(action_arg.str);

        Mock::VerifyAndClearExpectations(&mock_handlers);
    }
}

TEST_F(ARepowerdService,
       does_not_forward_set_critical_power_behavior_request_with_unsupported_actions)
{
    PowerArg<repowerd::PowerAction> power_action_args[] = {
        { repowerd::PowerAction::none, "invalid" },
        { repowerd::PowerAction::none, "none" },
        { repowerd::PowerAction::display_off, "display-off" }};

    for (auto const& action_arg : power_action_args)
    {
        EXPECT_CALL(mock_handlers, set_critical_power_behavior(_, _)).Times(0);

        try
        {
            client.request_set_critical_power_behavior(action_arg.str).get();
        }
        catch (...)
        {
        }

        Mock::VerifyAndClearExpectations(&mock_handlers);
    }
}

TEST_F(ARepowerdService, logs_set_critical_power_behavior_request)
{
    PowerArg<repowerd::PowerAction> power_action_args[] = {
        { repowerd::PowerAction::none, "invalid" },
        { repowerd::PowerAction::none, "none" },
        { repowerd::PowerAction::display_off, "display-off" },
        { repowerd::PowerAction::suspend, "suspend" },
        { repowerd::PowerAction::power_off, "power-off" }};

    for (auto const& action_arg : power_action_args)
    {
        try
        {
            client.request_set_critical_power_behavior(action_arg.str).get();
        }
        catch (...)
        {
        }

        EXPECT_TRUE(
            fake_log.contains_line({"SetCriticalPowerBehavior", action_arg.str}));
    }
}
