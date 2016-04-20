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
#include "src/adapters/device_config.h"
#include "src/adapters/powerd_service.h"
#include "src/adapters/unity_screen_service.h"

#include "dbus_bus.h"
#include "dbus_client.h"

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

struct FakeDeviceConfig : repowerd::DeviceConfig
{
    std::string get(
        std::string const& name, std::string const& default_prop_value) const override
    {
        if (name == "screenBrightnessDim")
            return std::to_string(dim_value);
        else if (name == "screenBrightnessSettingMininum")
            return std::to_string(min_value);
        else if (name == "screenBrightnessSettingMaximum")
            return std::to_string(max_value);
        else if (name == "screenBrightnessSettingDefault")
            return std::to_string(default_value);
        else if (name == "automatic_brightness_available")
            return "true";
        else
            return default_prop_value;
    }

    int const dim_value = 5;
    int const min_value = 2;
    int const max_value = 100;
    int const default_value = 50;
    bool const autobrightness_supported = true;
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
        registrations.push_back(
            unity_screen_service.register_set_inactivity_timeout_handler(
                [this] (std::chrono::milliseconds ms)
                {
                    mock_handlers.set_inactivity_timeout(ms);
                }));

        registrations.push_back(
            unity_screen_service.register_disable_autobrightness_handler(
                [this] { mock_handlers.disable_autobrightness(); }));
        registrations.push_back(
            unity_screen_service.register_enable_autobrightness_handler(
                [this] { mock_handlers.enable_autobrightness(); }));
        registrations.push_back(
            unity_screen_service.register_set_normal_brightness_value_handler(
                [this] (float v)
                {
                    mock_handlers.set_normal_brightness_value(v);
                }));

        registrations.push_back(
            unity_screen_service.register_notification_handler(
                [this] { mock_handlers.notification(); }));
        registrations.push_back(
            unity_screen_service.register_no_notification_handler(
                [this] { mock_handlers.no_notification(); }));
    }

    struct MockHandlers
    {
        MOCK_METHOD0(disable_inactivity_timeout, void());
        MOCK_METHOD0(enable_inactivity_timeout, void());
        MOCK_METHOD1(set_inactivity_timeout, void(std::chrono::milliseconds));

        MOCK_METHOD0(disable_autobrightness, void());
        MOCK_METHOD0(enable_autobrightness, void());
        MOCK_METHOD1(set_normal_brightness_value, void(float));

        MOCK_METHOD0(notification, void());
        MOCK_METHOD0(no_notification, void());
    };
    testing::NiceMock<MockHandlers> mock_handlers;

    static int constexpr active_state{1};
    std::chrono::seconds const default_timeout{3};

    rt::DBusBus bus;
    repowerd::UnityScreenService unity_screen_service{bus.address()};
    std::shared_ptr<repowerd::UnityScreenService> unity_screen_service_ptr{
        &unity_screen_service, [](void*){}};
    FakeDeviceConfig fake_device_config;
    repowerd::PowerdService powerd_service{
        unity_screen_service_ptr, fake_device_config, bus.address()};
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

    auto cookie = client.request_request_sys_state(active_state).get();
    EXPECT_THAT(cookie, StrNe(""));
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

TEST_F(APowerdService, forwards_clear_sys_state_request)
{
    auto cookie = client.request_request_sys_state(active_state).get();

    EXPECT_CALL(mock_handlers, enable_inactivity_timeout());
    client.request_clear_sys_state(cookie);
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

    EXPECT_THAT(dim_value, Eq(fake_device_config.dim_value));
    EXPECT_THAT(min_value, Eq(fake_device_config.min_value));
    EXPECT_THAT(max_value, Eq(fake_device_config.max_value));
    EXPECT_THAT(default_value, Eq(fake_device_config.default_value));
    EXPECT_THAT(autobrightness_supported, Eq(fake_device_config.autobrightness_supported));
}
