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
#include "src/adapters/temporary_suspend_inhibition.h"
#include "src/adapters/unity_screen_service.h"

#include "dbus_bus.h"
#include "dbus_client.h"
#include "fake_brightness_notification.h"
#include "fake_device_config.h"
#include "fake_log.h"
#include "fake_wakeup_service.h"

#include "fake_shared.h"
#include "wait_condition.h"
#include "spin_wait.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>
#include <cmath>
#include <functional>

using namespace testing;

namespace rt = repowerd::test;

namespace
{

char const* const powerd_service_name = "com.canonical.powerd";
char const* const powerd_path = "/com/canonical/powerd";
char const* const powerd_interface = "com.canonical.powerd";

struct MockTemporarySuspendInhibition : repowerd::TemporarySuspendInhibition
{
    MOCK_METHOD2(inhibit_suspend_for, void(std::chrono::milliseconds,std::string const&));
};

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

    rt::DBusAsyncReplyVoid request_clear_sys_state(std::string const& cookie)
    {
        return invoke_with_reply<rt::DBusAsyncReplyVoid>(
            powerd_interface, "clearSysState",
            g_variant_new("(s)", cookie.c_str()));
    }

    rt::DBusAsyncReplyString request_request_wakeup(
        std::chrono::system_clock::time_point tp)
    {
        auto const t64 = static_cast<uint64_t>(std::chrono::system_clock::to_time_t(tp));
        return invoke_with_reply<rt::DBusAsyncReplyString>(
            powerd_interface, "requestWakeup",
            g_variant_new("(st)", "test", t64));
    }

    rt::DBusAsyncReplyVoid request_clear_wakeup(std::string const& cookie)
    {
        return invoke_with_reply<rt::DBusAsyncReplyVoid>(
            powerd_interface, "clearWakeup",
            g_variant_new("(s)", cookie.c_str()));
    }

    rt::DBusAsyncReply request_get_brightness_params()
    {
        return invoke_with_reply<rt::DBusAsyncReply>(
            powerd_interface, "getBrightnessParams", nullptr);
    }

    repowerd::HandlerRegistration register_wakeup_handler(
        std::function<void()> const& func)
    {
        return event_loop.register_signal_handler(
            connection,
            nullptr,
            powerd_interface,
            "Wakeup",
            powerd_path,
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

    repowerd::HandlerRegistration register_brightness_handler(
        std::function<void(int32_t)> const& func)
    {
        return event_loop.register_signal_handler(
            connection,
            nullptr,
            "org.freedesktop.DBus.Properties",
            "PropertiesChanged",
            powerd_path,
            [func, this] (
                GDBusConnection* /*connection*/,
                gchar const* /*sender*/,
                gchar const* /*object_path*/,
                gchar const* /*interface_name*/,
                gchar const* /*signal_name*/,
                GVariant* parameters)
            {
                char const* interface_name{""};
                GVariantIter* changed_properties;
                GVariantIter* invalid_properties;

                g_variant_get(parameters, "(&sa{sv}as)",
                              &interface_name, &changed_properties, &invalid_properties);

                auto const brightness = get_brightness_property(changed_properties);

                g_variant_iter_free(changed_properties);
                g_variant_iter_free(invalid_properties);

                if (brightness >= 0)
                    func(brightness);
            });
    }

    int32_t get_brightness_property(GVariantIter* properties)
    {
        char const* prop_name_cstr{""};
        GVariant* prop_value{nullptr};
        int32_t brightness = -1;
        bool done = false;

        while (!done &&
               g_variant_iter_next(properties, "{&sv}", &prop_name_cstr, &prop_value))
        {
            std::string const prop_name{prop_name_cstr ? prop_name_cstr : ""};
            if (prop_name == "brightness")
            {
                brightness = g_variant_get_int32(prop_value);
                done = true;
            }
            g_variant_unref(prop_value);
        }

        return brightness;
    }
};

struct APowerdService : testing::Test
{
    APowerdService()
    {
        registrations.push_back(
            unity_screen_service.register_allow_suspend_handler(
                [this](auto id, auto pid){ mock_handlers.allow_suspend(id, pid); }));
        registrations.push_back(
            unity_screen_service.register_disallow_suspend_handler(
                [this](auto id, auto pid) { mock_handlers.disallow_suspend(id, pid); }));
        unity_screen_service.start_processing();
    }

    struct MockHandlers
    {
        MOCK_METHOD2(allow_suspend, void(std::string const&, pid_t));
        MOCK_METHOD2(disallow_suspend, void(std::string const&, pid_t));
    };
    testing::NiceMock<MockHandlers> mock_handlers;

    static int constexpr active_state{1};
    std::chrono::seconds const default_timeout{3};

    rt::DBusBus bus;
    rt::FakeBrightnessNotification fake_brightness_notification;
    rt::FakeDeviceConfig fake_device_config;
    rt::FakeLog fake_log;
    rt::FakeWakeupService fake_wakeup_service;
    NiceMock<MockTemporarySuspendInhibition> mock_temporary_suspend_inhibition;
    repowerd::UnityScreenService unity_screen_service{
        rt::fake_shared(fake_wakeup_service),
        rt::fake_shared(fake_brightness_notification),
        rt::fake_shared(fake_log),
        rt::fake_shared(mock_temporary_suspend_inhibition),
        fake_device_config,
        bus.address()};
    PowerdDBusClient client{bus.address()};
    std::vector<repowerd::HandlerRegistration> registrations;
};

ACTION_P(AppendArg0To, dst) { return dst->push_back(arg0); }

}

TEST_F(APowerdService, replies_to_introspection_request)
{
    auto reply = client.request_introspection().get();
    EXPECT_THAT(reply, StrNe(""));
}

TEST_F(APowerdService, forwards_request_sys_state_request)
{
    EXPECT_CALL(mock_handlers, disallow_suspend(_, _));

    client.request_request_sys_state(active_state).get();
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

TEST_F(APowerdService, allows_suspend_for_clear_sys_state_request)
{
    std::string id_disallow;
    std::string id_allow;

    InSequence s;
    EXPECT_CALL(mock_handlers, disallow_suspend(_, _))
        .WillOnce(SaveArg<0>(&id_disallow));
    EXPECT_CALL(mock_handlers, allow_suspend(_, _))
        .WillOnce(SaveArg<0>(&id_allow));

    auto reply1 = client.request_request_sys_state(active_state);
    client.request_clear_sys_state(reply1.get());

    EXPECT_THAT(id_allow, Eq(id_disallow));
}

TEST_F(APowerdService, allows_suspend_for_multiple_clear_sys_state_requests)
{
    std::vector<std::string> id_disallow;
    std::vector<std::string> id_allow;

    EXPECT_CALL(mock_handlers, disallow_suspend(_, _))
        .Times(3).WillRepeatedly(AppendArg0To(&id_disallow));
    EXPECT_CALL(mock_handlers, allow_suspend(_, _))
        .Times(2).WillRepeatedly(AppendArg0To(&id_allow));

    auto reply1 = client.request_request_sys_state(active_state);
    auto reply2 = client.request_request_sys_state(active_state);
    auto reply3 = client.request_request_sys_state(active_state);

    client.request_clear_sys_state(reply1.get());
    client.request_clear_sys_state(reply2.get());
    auto id3 = reply3.get();

    Mock::VerifyAndClearExpectations(&mock_handlers);

    EXPECT_CALL(mock_handlers, allow_suspend(_, _))
        .WillOnce(AppendArg0To(&id_allow));

    client.request_clear_sys_state(id3);

    EXPECT_THAT(id_allow, ContainerEq(id_disallow));
}

TEST_F(APowerdService, removes_all_client_suspend_disallowances_when_client_disconnects)
{
    std::vector<std::string> id_disallow;
    std::vector<std::string> id_allow;

    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, disallow_suspend(_, _))
        .Times(3).WillRepeatedly(AppendArg0To(&id_disallow));
    EXPECT_CALL(mock_handlers, allow_suspend(_, _))
        .WillOnce(AppendArg0To(&id_allow))
        .WillOnce(AppendArg0To(&id_allow))
        .WillOnce(DoAll(AppendArg0To(&id_allow), WakeUp(&request_processed)));

    client.request_request_sys_state(active_state);
    client.request_request_sys_state(active_state);
    client.request_request_sys_state(active_state);

    client.disconnect();

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());

    EXPECT_THAT(id_allow, UnorderedElementsAreArray(id_disallow));
}

TEST_F(APowerdService, ignores_invalid_clear_sys_state_request)
{
    std::string const invalid_cookie{"aaa"};

    EXPECT_CALL(mock_handlers, allow_suspend(_, _)).Times(0);

    client.request_clear_sys_state(invalid_cookie).get();
}

TEST_F(APowerdService, ignores_disconnects_from_clients_without_sys_state_request)
{
    EXPECT_CALL(mock_handlers, allow_suspend(_, _)).Times(0);

    client.disconnect();

    // Allow some time for disconnect notification to reach us
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(APowerdService, schedules_wakeup)
{
    auto const tp = std::chrono::system_clock::from_time_t(12345);

    client.request_request_wakeup(tp).get();

    EXPECT_THAT(fake_wakeup_service.emit_next_wakeup(), Eq(tp));
}

TEST_F(APowerdService, emits_wakeup_signal)
{
    auto const tp = std::chrono::system_clock::from_time_t(12345);

    std::promise<void> wakeup_promise;
    auto wakeup_future = wakeup_promise.get_future();

    auto const reg = client.register_wakeup_handler([&] { wakeup_promise.set_value(); });

    client.request_request_wakeup(tp).get();
    fake_wakeup_service.emit_next_wakeup();

    EXPECT_THAT(wakeup_future.wait_for(default_timeout),
                Eq(std::future_status::ready));
}

TEST_F(APowerdService, clears_wakeup)
{
    auto const tp = std::chrono::system_clock::from_time_t(12345);

    std::promise<void> wakeup_promise;
    auto wakeup_future = wakeup_promise.get_future();

    auto const reg = client.register_wakeup_handler([&] { wakeup_promise.set_value(); });

    auto const cookie = client.request_request_wakeup(tp).get();
    client.request_clear_wakeup(cookie).get();
    fake_wakeup_service.emit_next_wakeup();

    EXPECT_THAT(wakeup_future.wait_for(std::chrono::milliseconds{100}),
                Eq(std::future_status::timeout));
}

TEST_F(APowerdService, inhibits_suspend_temporarily_when_wakeup_is_triggered)
{
    auto const tp = std::chrono::system_clock::from_time_t(12345);

    client.request_request_wakeup(tp).get();

    auto const inhibition_timeout = std::chrono::milliseconds{3000};
    EXPECT_CALL(mock_temporary_suspend_inhibition,
                inhibit_suspend_for(inhibition_timeout, _));

    fake_wakeup_service.emit_next_wakeup();
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

TEST_F(APowerdService, emits_brightness_property_change)
{
    std::promise<int32_t> brightness_promise;
    auto brightness_future = brightness_promise.get_future();

    auto const reg = client.register_brightness_handler(
        [&](int32_t brightness) { brightness_promise.set_value(brightness); });

    fake_brightness_notification.emit_brightness(0.7);

    ASSERT_THAT(brightness_future.wait_for(default_timeout),
                Eq(std::future_status::ready));
    EXPECT_THAT(brightness_future.get(), Eq(round(0.7 * fake_device_config.brightness_max_value)));
}

TEST_F(APowerdService, logs_request_sys_state_request)
{
    auto const cookie = client.request_request_sys_state(active_state).get();

    EXPECT_TRUE(
        fake_log.contains_line({
            "requestSysState",
            "test",
            std::to_string(active_state),
            client.unique_name(),
            cookie}));
}

TEST_F(APowerdService, logs_clear_sys_state_request)
{
    auto const cookie = client.request_request_sys_state(active_state).get();
    client.request_clear_sys_state(cookie).get();

    EXPECT_TRUE(
        fake_log.contains_line({
            "clearSysState",
            client.unique_name(),
            cookie}));
}

TEST_F(APowerdService, logs_request_wakeup_request)
{
    auto const tp = std::chrono::system_clock::from_time_t(12345);

    auto const cookie = client.request_request_wakeup(tp).get();

    EXPECT_TRUE(
        fake_log.contains_line({
            "requestWakeup",
            client.unique_name(),
            "test",
            cookie,
            std::to_string(std::chrono::system_clock::to_time_t(tp))}));
}

TEST_F(APowerdService, logs_clear_wakeup_request)
{
    auto const tp = std::chrono::system_clock::from_time_t(12345);
    auto const cookie = client.request_request_wakeup(tp).get();
    client.request_clear_wakeup(cookie).get();

    EXPECT_TRUE(
        fake_log.contains_line({
            "clearWakeup",
            client.unique_name(),
            cookie}));
}

TEST_F(APowerdService, logs_get_brightness_params)
{
    client.request_get_brightness_params().get();

    EXPECT_TRUE(
        fake_log.contains_line({
            "getBrightnessParams",
            std::to_string(fake_device_config.brightness_dim_value),
            std::to_string(fake_device_config.brightness_min_value),
            std::to_string(fake_device_config.brightness_max_value),
            std::to_string(fake_device_config.brightness_default_value),
            fake_device_config.brightness_autobrightness_supported ? "true" : "false"
            }));
}

TEST_F(APowerdService, logs_wakeup_signal)
{
    auto const tp = std::chrono::system_clock::from_time_t(12345);

    std::promise<void> wakeup_promise;
    auto wakeup_future = wakeup_promise.get_future();

    auto const reg = client.register_wakeup_handler([&] { wakeup_promise.set_value(); });

    client.request_request_wakeup(tp).get();
    fake_wakeup_service.emit_next_wakeup();

    EXPECT_THAT(wakeup_future.wait_for(default_timeout),
                Eq(std::future_status::ready));

    EXPECT_TRUE(fake_log.contains_line({"emit", "Wakeup"}));
}

TEST_F(APowerdService, logs_brightness_signal)
{
    double const brightness = 0.7;
    std::promise<void> brightness_promise;
    auto brightness_future = brightness_promise.get_future();

    auto const reg = client.register_brightness_handler(
        [&](int32_t) { brightness_promise.set_value(); });

    fake_brightness_notification.emit_brightness(brightness);

    ASSERT_THAT(brightness_future.wait_for(default_timeout),
                Eq(std::future_status::ready));

    EXPECT_TRUE(
        fake_log.contains_line({
            "emit", "brightness",
            std::to_string(brightness),
            std::to_string(
                static_cast<int>(round(brightness * fake_device_config.brightness_max_value)))
            }));
}
