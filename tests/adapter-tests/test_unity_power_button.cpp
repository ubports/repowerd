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
#include "src/adapters/unity_power_button.h"

#include "wait_condition.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>
#include <functional>

namespace rt = repowerd::test;
using namespace std::chrono_literals;

namespace
{

struct UnityPowerButtonDBusClient : rt::DBusClient
{
    UnityPowerButtonDBusClient(std::string const& dbus_address)
        : rt::DBusClient{
            dbus_address,
            "com.canonical.Unity.PowerButton",
            "/com/canonical/Unity/PowerButton"}
    {
        connection.request_name("com.canonical.Unity.PowerButton");
    }

    void emit_power_button_press()
    {
        emit_signal("com.canonical.Unity.PowerButton", "Press", nullptr);
    }

    void emit_power_button_release()
    {
        emit_signal("com.canonical.Unity.PowerButton", "Release", nullptr);
    }

    repowerd::HandlerRegistration register_long_press_handler(
        std::function<void()> const& func)
    {
        return event_loop.register_signal_handler(
            connection,
            nullptr,
            "com.canonical.Unity.PowerButton",
            "LongPress",
            "/com/canonical/Unity/PowerButton",
            [func] (
                GDBusConnection* /*connection*/,
                gchar const* /*sender*/,
                gchar const* /*object_path*/,
                gchar const* /*interface_name*/,
                gchar const* /*signal_name*/,
                GVariant* /*parameters*/)
            {
                func();
            });
    }
};

struct AUnityPowerButton : testing::Test
{
    AUnityPowerButton()
    {
        registrations.push_back(
            unity_power_button.register_power_button_handler(
                [this] (repowerd::PowerButtonState state)
                {
                    mock_handlers.power_button(state);
                }));
        unity_power_button.start_processing();
    }

    struct MockHandlers
    {
        MOCK_METHOD1(power_button, void(repowerd::PowerButtonState));
    };
    testing::NiceMock<MockHandlers> mock_handlers;

    rt::DBusBus bus;
    repowerd::UnityPowerButton unity_power_button{bus.address()};
    UnityPowerButtonDBusClient client{bus.address()};
    std::vector<repowerd::HandlerRegistration> registrations;

    std::chrono::seconds const default_timeout{3};
};

}

TEST_F(AUnityPowerButton, calls_handler_for_power_button_press_signal)
{
    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, power_button(repowerd::PowerButtonState::pressed))
        .WillOnce(WakeUp(&request_processed));

    client.emit_power_button_press();

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());
}

TEST_F(AUnityPowerButton, calls_handler_for_power_button_release_signal)
{
    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, power_button(repowerd::PowerButtonState::released))
        .WillOnce(WakeUp(&request_processed));

    client.emit_power_button_release();

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());
}

TEST_F(AUnityPowerButton, does_not_calls_unregistered_handlers)
{
    using namespace testing;

    registrations.clear();

    EXPECT_CALL(mock_handlers, power_button(_)).Times(0);

    client.emit_power_button_press();
    client.emit_power_button_release();

    // Give some time for dbus signals to be delivered
    std::this_thread::sleep_for(100ms);
}

TEST_F(AUnityPowerButton, emits_long_press_signal)
{
    using namespace testing;

    std::promise<void> promise;
    auto future = promise.get_future();

    auto const reg = client.register_long_press_handler(
        [&promise] { promise.set_value(); });

    unity_power_button.notify_long_press();

    EXPECT_THAT(future.wait_for(default_timeout), std::future_status::ready);
}
