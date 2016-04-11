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
#include "src/adapters/unity_user_activity.h"

#include "wait_condition.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>

namespace rt = repowerd::test;
using namespace std::chrono_literals;

namespace
{

struct UnityUserActivityDBusClient : rt::DBusClient
{
    UnityUserActivityDBusClient(std::string const& dbus_address)
        : rt::DBusClient{
            dbus_address,
            "com.canonical.Unity.UserActivity",
            "/com/canonical/Unity/UserActivity"}
    {
        connection.request_name("com.canonical.Unity.UserActivity");
    }

    void emit_user_activity_changing_power_state()
    {
        emit_signal("com.canonical.Unity.UserActivity", "Activity", g_variant_new("(i)", 0));
    }

    void emit_user_activity_extending_power_state()
    {
        emit_signal("com.canonical.Unity.UserActivity", "Activity", g_variant_new("(i)", 1));
    }
};

struct AUnityUserActivity : testing::Test
{
    AUnityUserActivity()
    {
        registrations.push_back(
            unity_user_activity.register_user_activity_handler(
                [this] (repowerd::UserActivityType type)
                {
                    mock_handlers.user_activity(type);
                }));
    }

    struct MockHandlers
    {
        MOCK_METHOD1(user_activity, void(repowerd::UserActivityType));
    };
    testing::NiceMock<MockHandlers> mock_handlers;

    rt::DBusBus bus;
    repowerd::UnityUserActivity unity_user_activity{bus.address()};
    UnityUserActivityDBusClient client{bus.address()};
    std::vector<repowerd::HandlerRegistration> registrations;

    std::chrono::seconds const default_timeout{3};
};

}

TEST_F(AUnityUserActivity, calls_handler_for_user_activity_changing_power_state)
{
    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, user_activity(repowerd::UserActivityType::change_power_state))
        .WillOnce(WakeUp(&request_processed));

    client.emit_user_activity_changing_power_state();

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());
}

TEST_F(AUnityUserActivity, calls_handler_for_user_activity_extending_power_state)
{
    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, user_activity(repowerd::UserActivityType::extend_power_state))
        .WillOnce(WakeUp(&request_processed));

    client.emit_user_activity_extending_power_state();

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());
}

TEST_F(AUnityUserActivity, does_not_calls_unregistered_handlers)
{
    using namespace testing;

    registrations.clear();

    EXPECT_CALL(mock_handlers, user_activity(_)).Times(0);

    client.emit_user_activity_changing_power_state();
    client.emit_user_activity_extending_power_state();

    // Give some time for dbus signals to be delivered
    std::this_thread::sleep_for(10ms);
}
