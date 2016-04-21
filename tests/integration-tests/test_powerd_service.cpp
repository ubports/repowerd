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

#include "src/adapters/dbus_connection_handle.h"
#include "src/adapters/dbus_message_handle.h"
#include "src/adapters/unity_screen_service.h"

#include "dbus_bus.h"
#include "dbus_client.h"
#include "fake_device_config.h"

#include "wait_condition.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>

using namespace testing;

namespace rt = repowerd::test;

namespace
{

char const* const powerd_service_name = "com.canonical.powerd";
char const* const powerd_path = "/com/canonical/powerd";
char const* const powerd_interface = "com.canonical.powerd";

struct PowerdDBusClient : rt::DBusClient
{
    PowerdDBusClient(std::string const& dbus_address)
        : rt::DBusClient{
            dbus_address,
            powerd_service_name,
            powerd_path}
    {
    }

    rt::DBusAsyncReplyString request_introspection()
    {
        return invoke_with_reply<rt::DBusAsyncReplyString>(
            "org.freedesktop.DBus.Introspectable", "Introspect",
            nullptr);
    }

    rt::DBusAsyncReplyString request_request_sys_state(int32_t state)
    {
        return invoke_with_reply<rt::DBusAsyncReplyString>(
            powerd_interface, "requestSysState",
            g_variant_new("(si)", "test", state));
    }

    rt::DBusAsyncReplyString request_clear_sys_state(std::string const& cookie)
    {
        return invoke_with_reply<rt::DBusAsyncReplyString>(
            powerd_interface, "clearSysState",
            g_variant_new("(s)", cookie.c_str()));
    }

    rt::DBusAsyncReply request_get_brightness_params()
    {
        return invoke_with_reply<rt::DBusAsyncReply>(
            powerd_interface, "getBrightnessParams", nullptr);
    }
};

struct APowerdService : testing::Test
{
    APowerdService()
    {
        registrations.push_back(
            unity_screen_service.register_disable_inactivity_timeout_handler(
                [this] { mock_handlers.disable_inactivity_timeout(); }));
        registrations.push_back(
            unity_screen_service.register_enable_inactivity_timeout_handler(
                [this] { mock_handlers.enable_inactivity_timeout(); }));

        unity_screen_service.start_processing();
    }

    struct MockHandlers
    {
        MOCK_METHOD0(disable_inactivity_timeout, void());
        MOCK_METHOD0(enable_inactivity_timeout, void());
    };
    testing::NiceMock<MockHandlers> mock_handlers;

    static int constexpr active_state{1};
    std::chrono::seconds const default_timeout{3};

    rt::DBusBus bus;
    rt::FakeDeviceConfig fake_device_config;
    repowerd::UnityScreenService unity_screen_service{
        fake_device_config, bus.address()};
    PowerdDBusClient client{bus.address()};
    std::vector<repowerd::HandlerRegistration> registrations;
};

}

TEST_F(APowerdService, replies_to_introspection_request)
{
    auto reply = client.request_introspection().get();
    EXPECT_THAT(reply, StrNe(""));
}

TEST_F(APowerdService, forwards_request_sys_state_request)
{
    EXPECT_CALL(mock_handlers, disable_inactivity_timeout());

    client.request_request_sys_state(active_state);
}

TEST_F(APowerdService, returns_different_cookies_for_request_sys_state_requests)
{
    auto cookie1 = client.request_request_sys_state(active_state).get();
    auto cookie2 = client.request_request_sys_state(active_state).get();

    EXPECT_THAT(cookie1, StrNe(""));
    EXPECT_THAT(cookie2, StrNe(""));
    EXPECT_THAT(cookie1, StrNe(cookie2));
}

TEST_F(APowerdService, returns_error_for_invalid_request_sys_state_request)
{
    auto const invalid_state = 0;
    auto cookie = client.request_request_sys_state(invalid_state);
    auto reply_msg = static_cast<rt::DBusAsyncReply&>(cookie).get();

    EXPECT_THAT(g_dbus_message_get_message_type(reply_msg), Eq(G_DBUS_MESSAGE_TYPE_ERROR));
}

TEST_F(APowerdService,
       reenables_inactivity_timeout_when_single_request_sys_state_request_is_cleared)
{
    using namespace testing;

    InSequence s;
    EXPECT_CALL(mock_handlers, disable_inactivity_timeout());
    EXPECT_CALL(mock_handlers, enable_inactivity_timeout());

    auto reply1 = client.request_request_sys_state(active_state);
    client.request_clear_sys_state(reply1.get());
}

TEST_F(APowerdService,
       reenables_inactivity_timeout_when_all_request_sys_state_request_are_cleared)
{
    using namespace testing;

    EXPECT_CALL(mock_handlers, disable_inactivity_timeout()).Times(3);
    EXPECT_CALL(mock_handlers, enable_inactivity_timeout()).Times(0);

    auto reply1 = client.request_request_sys_state(active_state);
    auto reply2 = client.request_request_sys_state(active_state);
    auto reply3 = client.request_request_sys_state(active_state);

    client.request_clear_sys_state(reply1.get());
    client.request_clear_sys_state(reply2.get());
    auto cookie3 = reply3.get();

    // Display should still be kept on at this point
    Mock::VerifyAndClearExpectations(&mock_handlers);

    // keep_display_on should be disable only when the last request is removed
    EXPECT_CALL(mock_handlers, enable_inactivity_timeout());

    client.request_clear_sys_state(cookie3);
}

TEST_F(APowerdService,
       reenables_inactivity_timeout_when_single_client_disconnects)
{
    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, disable_inactivity_timeout()).Times(3);
    EXPECT_CALL(mock_handlers, enable_inactivity_timeout())
        .WillOnce(WakeUp(&request_processed));

    client.request_request_sys_state(active_state);
    client.request_request_sys_state(active_state);
    client.request_request_sys_state(active_state);

    client.disconnect();

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());
}

TEST_F(APowerdService,
       reenables_inactivity_timeout_when_all_clients_disconnect_or_remove_requests)
{
    using namespace testing;

    PowerdDBusClient other_client{bus.address()};

    EXPECT_CALL(mock_handlers, disable_inactivity_timeout()).Times(4);
    EXPECT_CALL(mock_handlers, enable_inactivity_timeout()).Times(0);

    auto reply1 = client.request_request_sys_state(active_state);
    auto reply2 = client.request_request_sys_state(active_state);
    other_client.request_request_sys_state(active_state);
    other_client.request_request_sys_state(active_state);

    other_client.disconnect();
    client.request_clear_sys_state(reply1.get());
    auto cookie2 = reply2.get();

    // Display should still be kept on at this point
    Mock::VerifyAndClearExpectations(&mock_handlers);

    // keep_display_on should be disabled only when the last request is removed
    rt::WaitCondition request_processed;
    EXPECT_CALL(mock_handlers, enable_inactivity_timeout())
        .WillOnce(WakeUp(&request_processed));

    client.request_clear_sys_state(cookie2);

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());
}

TEST_F(APowerdService, ignores_invalid_clear_sys_state_request)
{
    std::string const invalid_cookie{"aaa"};
    EXPECT_CALL(mock_handlers, enable_inactivity_timeout()).Times(0);

    client.request_clear_sys_state(invalid_cookie);
    client.disconnect();

    // Allow some time for dbus calls to reach UnityScreenService
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(APowerdService, ignores_disconnects_from_clients_without_sys_state_request)
{
    EXPECT_CALL(mock_handlers, enable_inactivity_timeout()).Times(0);

    client.disconnect();

    // Allow some time for disconnect notification to reach UnityScreenService
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(APowerdService, replies_to_get_brightness_params_request)
{
    auto params = client.request_get_brightness_params().get();
    auto body = g_dbus_message_get_body(params);

    int32_t dim_value;
    int32_t min_value;
    int32_t max_value;
    int32_t default_value;
    gboolean autobrightness_supported;

    g_variant_get(
        body, "((iiiib))", &dim_value, &min_value, &max_value,
        &default_value, &autobrightness_supported);

    EXPECT_THAT(dim_value, Eq(fake_device_config.brightness_dim_value));
    EXPECT_THAT(min_value, Eq(fake_device_config.brightness_min_value));
    EXPECT_THAT(max_value, Eq(fake_device_config.brightness_max_value));
    EXPECT_THAT(default_value, Eq(fake_device_config.brightness_default_value));
    EXPECT_THAT(autobrightness_supported,
                Eq(fake_device_config.brightness_autobrightness_supported));
}
