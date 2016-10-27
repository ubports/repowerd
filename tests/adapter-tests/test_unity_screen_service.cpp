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
#include "fake_brightness_notification.h"
#include "fake_device_config.h"
#include "fake_log.h"
#include "fake_suspend_control.h"
#include "fake_wakeup_service.h"
#include "unity_screen_dbus_client.h"
#include "src/adapters/dbus_connection_handle.h"
#include "src/adapters/dbus_message_handle.h"
#include "src/adapters/unity_screen_power_state_change_reason.h"
#include "src/adapters/unity_screen_service.h"
#include "src/core/infinite_timeout.h"

#include "fake_shared.h"
#include "wait_condition.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>

using namespace testing;

namespace rt = repowerd::test;

namespace
{

struct AUnityScreenService : testing::Test
{
    AUnityScreenService()
    {
        registrations.push_back(
            service.register_disable_inactivity_timeout_handler(
                [this](auto id){ mock_handlers.disable_inactivity_timeout(id); }));
        registrations.push_back(
            service.register_enable_inactivity_timeout_handler(
                [this](auto id) { mock_handlers.enable_inactivity_timeout(id); }));
        registrations.push_back(
            service.register_set_inactivity_timeout_handler(
                [this] (std::chrono::milliseconds ms)
                {
                    mock_handlers.set_inactivity_timeout(ms);
                }));

        registrations.push_back(
            service.register_disable_autobrightness_handler(
                [this] { mock_handlers.disable_autobrightness(); }));
        registrations.push_back(
            service.register_enable_autobrightness_handler(
                [this] { mock_handlers.enable_autobrightness(); }));
        registrations.push_back(
            service.register_set_normal_brightness_value_handler(
                [this] (double v)
                {
                    mock_handlers.set_normal_brightness_value(v);
                }));

        registrations.push_back(
            service.register_notification_handler(
                [this](auto id) { mock_handlers.notification(id); }));
        registrations.push_back(
            service.register_notification_done_handler(
                [this](auto id) { mock_handlers.notification_done(id); }));

        service.start_processing();
    }

    struct MockHandlers
    {
        MOCK_METHOD1(disable_inactivity_timeout, void(std::string const&));
        MOCK_METHOD1(enable_inactivity_timeout, void(std::string const&));
        MOCK_METHOD1(set_inactivity_timeout, void(std::chrono::milliseconds));

        MOCK_METHOD0(disable_autobrightness, void());
        MOCK_METHOD0(enable_autobrightness, void());
        MOCK_METHOD1(set_normal_brightness_value, void(double));

        MOCK_METHOD1(notification, void(std::string const&));
        MOCK_METHOD1(notification_done, void(std::string const&));
    };
    testing::NiceMock<MockHandlers> mock_handlers;

    static int constexpr notification_reason{4};
    static int constexpr snap_decision_reason{5};
    std::chrono::seconds const default_timeout{3};

    rt::DBusBus bus;
    rt::FakeBrightnessNotification fake_brightness_notification;
    rt::FakeDeviceConfig fake_device_config;
    rt::FakeLog fake_log;
    rt::FakeSuspendControl fake_suspend_control;
    rt::FakeWakeupService fake_wakeup_service;
    repowerd::UnityScreenService service{
        rt::fake_shared(fake_wakeup_service),
        rt::fake_shared(fake_brightness_notification),
        rt::fake_shared(fake_log),
        rt::fake_shared(fake_suspend_control),
        fake_device_config,
        bus.address()};
    rt::UnityScreenDBusClient client{bus.address()};
    std::vector<repowerd::HandlerRegistration> registrations;
};

ACTION_P(AppendArg0To, dst) { return dst->push_back(arg0); }

}

TEST_F(AUnityScreenService, replies_to_introspection_request)
{
    auto reply = client.request_introspection();
    EXPECT_THAT(reply.get(), StrNe(""));
}

TEST_F(AUnityScreenService, forwards_keep_display_on_request)
{
    EXPECT_CALL(mock_handlers, disable_inactivity_timeout(_));

    client.request_keep_display_on();
}

TEST_F(AUnityScreenService, replies_with_different_ids_to_keep_display_on_requests)
{
    auto reply1 = client.request_keep_display_on();
    auto reply2 = client.request_keep_display_on();

    auto const id1 = reply1.get();
    auto const id2 = reply2.get();

    EXPECT_THAT(id1, Ne(id2));
}

TEST_F(AUnityScreenService, disables_and_enables_inactivity_timeout_for_keep_display_on_request)
{
    std::string id_disable;
    std::string id_enable;

    InSequence s;
    EXPECT_CALL(mock_handlers, disable_inactivity_timeout(_))
        .WillOnce(SaveArg<0>(&id_disable));
    EXPECT_CALL(mock_handlers, enable_inactivity_timeout(_))
        .WillOnce(SaveArg<0>(&id_enable));

    auto reply1 = client.request_keep_display_on();
    client.request_remove_display_on_request(reply1.get());

    EXPECT_THAT(id_enable, Eq(id_disable));
}

TEST_F(AUnityScreenService, disables_and_enables_inactivity_timeout_for_multiple_keep_display_on_requests)
{
    std::vector<std::string> id_disable;
    std::vector<std::string> id_enable;

    EXPECT_CALL(mock_handlers, disable_inactivity_timeout(_))
        .Times(3).WillRepeatedly(AppendArg0To(&id_disable));
    EXPECT_CALL(mock_handlers, enable_inactivity_timeout(_))
        .Times(2).WillRepeatedly(AppendArg0To(&id_enable));

    auto reply1 = client.request_keep_display_on();
    auto reply2 = client.request_keep_display_on();
    auto reply3 = client.request_keep_display_on();

    client.request_remove_display_on_request(reply1.get());
    client.request_remove_display_on_request(reply2.get());
    auto id3 = reply3.get();

    Mock::VerifyAndClearExpectations(&mock_handlers);

    EXPECT_CALL(mock_handlers, enable_inactivity_timeout(_))
        .WillOnce(AppendArg0To(&id_enable));

    client.request_remove_display_on_request(id3);

    EXPECT_THAT(id_enable, ContainerEq(id_disable));
}

TEST_F(AUnityScreenService, removes_all_client_inactivity_disablements_when_client_disconnects)
{
    std::vector<std::string> id_disable;
    std::vector<std::string> id_enable;

    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, disable_inactivity_timeout(_))
        .Times(3).WillRepeatedly(AppendArg0To(&id_disable));
    EXPECT_CALL(mock_handlers, enable_inactivity_timeout(_))
        .WillOnce(AppendArg0To(&id_enable))
        .WillOnce(AppendArg0To(&id_enable))
        .WillOnce(DoAll(AppendArg0To(&id_enable), WakeUp(&request_processed)));

    client.request_keep_display_on();
    client.request_keep_display_on();
    client.request_keep_display_on();

    client.disconnect();

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());

    EXPECT_THAT(id_enable, UnorderedElementsAreArray(id_disable));
}

TEST_F(AUnityScreenService, removes_client_inactivity_disablements_when_all_clients_disconnect_or_remove_requests)
{
    std::vector<std::string> id_disable;
    std::vector<std::string> id_enable;

    rt::UnityScreenDBusClient other_client{bus.address()};

    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, disable_inactivity_timeout(_))
        .Times(4).WillRepeatedly(AppendArg0To(&id_disable));
    EXPECT_CALL(mock_handlers, enable_inactivity_timeout(_))
        .WillOnce(AppendArg0To(&id_enable))
        .WillOnce(AppendArg0To(&id_enable))
        .WillOnce(DoAll(AppendArg0To(&id_enable), WakeUp(&request_processed)));

    auto reply1 = client.request_keep_display_on();
    auto reply2 = client.request_keep_display_on();
    other_client.request_keep_display_on();
    other_client.request_keep_display_on();

    other_client.disconnect();
    client.request_remove_display_on_request(reply1.get());
    auto id2 = reply2.get();

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());

    Mock::VerifyAndClearExpectations(&mock_handlers);

    EXPECT_CALL(mock_handlers, enable_inactivity_timeout(_))
        .WillOnce(AppendArg0To(&id_enable));

    client.request_remove_display_on_request(id2);

    EXPECT_THAT(id_enable, UnorderedElementsAreArray(id_disable));
}

TEST_F(AUnityScreenService, ignores_invalid_display_on_removal_request)
{
    int32_t const invalid_id{-1};
    EXPECT_CALL(mock_handlers, enable_inactivity_timeout(_)).Times(0);

    client.request_remove_display_on_request(invalid_id);
    client.disconnect();

    // Allow some time for dbus calls to reach UnityScreenService
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(AUnityScreenService, ignores_disconnects_from_clients_without_display_on_request)
{
    EXPECT_CALL(mock_handlers, enable_inactivity_timeout(_)).Times(0);

    client.disconnect();

    // Allow some time for disconnect notification to reach UnityScreenService
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(AUnityScreenService, forwards_set_inactivity_timeouts_request)
{
    int32_t const poweroff_timeout = 10;
    int32_t const dimmer_timeout = 5;

    EXPECT_CALL(mock_handlers, set_inactivity_timeout(std::chrono::milliseconds{1000 * poweroff_timeout}));

    client.request_set_inactivity_timeouts(poweroff_timeout, dimmer_timeout);
}

TEST_F(AUnityScreenService, forwards_infinite_inactivity_timeouts_request)
{
    int32_t const infinite_poweroff_timeout = 0;
    int32_t const dimmer_timeout = 5;

    EXPECT_CALL(mock_handlers, set_inactivity_timeout(repowerd::infinite_timeout));

    client.request_set_inactivity_timeouts(infinite_poweroff_timeout, dimmer_timeout);
}

TEST_F(AUnityScreenService, ignores_negative_inactivity_timeouts_request)
{
    int32_t const negative_poweroff_timeout = -1;
    int32_t const dimmer_timeout = 5;

    EXPECT_CALL(mock_handlers, set_inactivity_timeout(_)).Times(0);

    client.request_set_inactivity_timeouts(negative_poweroff_timeout, dimmer_timeout);

    // Allow some time for dbus calls to reach UnityScreenService
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(AUnityScreenService, forwards_user_auto_brightness_disable_request)
{
    bool const disable = false;

    EXPECT_CALL(mock_handlers, disable_autobrightness());

    client.request_user_auto_brightness_enable(disable);
}

TEST_F(AUnityScreenService, forwards_user_auto_brightness_enable_request)
{
    bool const enable = true;

    EXPECT_CALL(mock_handlers, enable_autobrightness());

    client.request_user_auto_brightness_enable(enable);
}

TEST_F(AUnityScreenService, forwards_set_user_brightness_request)
{
    int const brightness = 10;
    double const brightness_normalized =
        brightness / static_cast<double>(fake_device_config.brightness_max_value);

    EXPECT_CALL(mock_handlers, set_normal_brightness_value(brightness_normalized));
    client.request_set_user_brightness(brightness);
}

TEST_F(AUnityScreenService, forwards_set_screen_power_mode_notification_on_request)
{
    EXPECT_CALL(mock_handlers, notification(_));

    auto reply = client.request_set_screen_power_mode("on", notification_reason);
    EXPECT_THAT(reply.get(), Eq(true));
}

TEST_F(AUnityScreenService,
       forwards_set_screen_power_mode_snap_decision_on_request_as_notification)
{
    EXPECT_CALL(mock_handlers, notification(_));

    auto reply = client.request_set_screen_power_mode("on", snap_decision_reason);
    EXPECT_THAT(reply.get(), Eq(true));
}

TEST_F(AUnityScreenService,
       notifies_of_notification_done_for_all_notifications_and_snap_decisions)
{
    std::vector<std::string> notification_ids;
    std::vector<std::string> notification_done_ids;

    EXPECT_CALL(mock_handlers, notification(_))
        .Times(3).WillRepeatedly(AppendArg0To(&notification_ids));
    client.request_set_screen_power_mode("on", notification_reason);
    client.request_set_screen_power_mode("on", snap_decision_reason);
    client.request_set_screen_power_mode("on", notification_reason);
    Mock::VerifyAndClearExpectations(&mock_handlers);

    EXPECT_CALL(mock_handlers, notification(_)).Times(0);
    EXPECT_CALL(mock_handlers, notification_done(_))
        .Times(3).WillRepeatedly(AppendArg0To(&notification_done_ids));
    client.request_set_screen_power_mode("off", notification_reason);
    client.request_set_screen_power_mode("off", notification_reason);
    client.request_set_screen_power_mode("off", snap_decision_reason);

    EXPECT_THAT(notification_done_ids, UnorderedElementsAreArray(notification_ids));
}

TEST_F(AUnityScreenService, notifies_of_notification_done_when_single_client_disconnects)
{
    std::vector<std::string> notification_ids;
    std::vector<std::string> notification_done_ids;

    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, notification(_))
        .Times(3).WillRepeatedly(AppendArg0To(&notification_ids));
    EXPECT_CALL(mock_handlers, notification_done(_))
        .WillOnce(AppendArg0To(&notification_done_ids))
        .WillOnce(AppendArg0To(&notification_done_ids))
        .WillOnce(DoAll(AppendArg0To(&notification_done_ids),WakeUp(&request_processed)));

    client.request_set_screen_power_mode("on", notification_reason);
    client.request_set_screen_power_mode("on", notification_reason);
    client.request_set_screen_power_mode("on", snap_decision_reason);

    client.disconnect();

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());

    EXPECT_THAT(notification_done_ids, UnorderedElementsAreArray(notification_ids));
}

TEST_F(AUnityScreenService,
       notifies_of_notification_done_for_all_clients_that_disconnect_or_remove_requests)
{
    std::vector<std::string> notification_ids;
    std::vector<std::string> notification_done_ids;

    rt::UnityScreenDBusClient other_client{bus.address()};

    EXPECT_CALL(mock_handlers, notification(_))
        .Times(4).WillRepeatedly(AppendArg0To(&notification_ids));

    client.request_set_screen_power_mode("on", notification_reason);
    client.request_set_screen_power_mode("on", snap_decision_reason);
    other_client.request_set_screen_power_mode("on", notification_reason);
    other_client.request_set_screen_power_mode("on", snap_decision_reason);

    Mock::VerifyAndClearExpectations(&mock_handlers);

    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, notification_done(_))
        .WillOnce(AppendArg0To(&notification_done_ids))
        .WillOnce(AppendArg0To(&notification_done_ids))
        .WillOnce(AppendArg0To(&notification_done_ids))
        .WillOnce(DoAll(AppendArg0To(&notification_done_ids), WakeUp(&request_processed)));

    other_client.disconnect();
    client.request_set_screen_power_mode("off", notification_reason);
    client.request_set_screen_power_mode("off", snap_decision_reason);

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());
}

TEST_F(AUnityScreenService, ignores_notification_done_request_from_client_without_notifications)
{
    EXPECT_CALL(mock_handlers, notification_done(_)).Times(0);
    client.request_set_screen_power_mode("off", notification_reason);
}

TEST_F(AUnityScreenService, ignores_disconnects_from_clients_without_notifications)
{
    EXPECT_CALL(mock_handlers, notification_done(_)).Times(0);

    client.disconnect();

    // Allow some time for disconnect notification to reach UnityScreenService
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(AUnityScreenService, returns_false_for_unsupported_set_screen_power_mode_request)
{
    static int constexpr unknown_reason = 0;

    auto reply1 = client.request_set_screen_power_mode("on", unknown_reason);
    auto reply2 = client.request_set_screen_power_mode("off", unknown_reason);

    EXPECT_THAT(reply1.get(), Eq(false));
    EXPECT_THAT(reply2.get(), Eq(false));
}

TEST_F(AUnityScreenService, returns_error_reply_for_invalid_method)
{
    auto reply = client.request_invalid_method();
    auto reply_msg = reply.get();

    EXPECT_THAT(g_dbus_message_get_message_type(reply_msg), Eq(G_DBUS_MESSAGE_TYPE_ERROR));
}

TEST_F(AUnityScreenService, returns_error_reply_for_method_with_invalid_interface)
{
    auto reply = client.request_method_with_invalid_interface();
    auto reply_msg = reply.get();

    EXPECT_THAT(g_dbus_message_get_message_type(reply_msg), Eq(G_DBUS_MESSAGE_TYPE_ERROR));
}

TEST_F(AUnityScreenService, returns_error_reply_for_method_with_invalid_arguments)
{
    auto reply = client.request_method_with_invalid_arguments();
    auto reply_msg = reply.get();

    EXPECT_THAT(g_dbus_message_get_message_type(reply_msg), Eq(G_DBUS_MESSAGE_TYPE_ERROR));
}

TEST_F(AUnityScreenService, returns_error_reply_for_set_touch_visualization_enabled_request)
{
    auto reply = client.request_set_touch_visualization_enabled(true);
    auto reply_msg = static_cast<rt::DBusAsyncReply*>(&reply)->get();

    EXPECT_THAT(g_dbus_message_get_message_type(reply_msg), Eq(G_DBUS_MESSAGE_TYPE_ERROR));
}

TEST_F(AUnityScreenService, does_not_call_unregistered_handlers)
{
    EXPECT_CALL(mock_handlers, disable_inactivity_timeout(_)).Times(0);
    EXPECT_CALL(mock_handlers, enable_inactivity_timeout(_)).Times(0);
    EXPECT_CALL(mock_handlers, set_inactivity_timeout(_)).Times(0);

    EXPECT_CALL(mock_handlers, disable_autobrightness()).Times(0);
    EXPECT_CALL(mock_handlers, enable_autobrightness()).Times(0);
    EXPECT_CALL(mock_handlers, set_normal_brightness_value(_)).Times(0);

    EXPECT_CALL(mock_handlers, notification(_)).Times(0);
    EXPECT_CALL(mock_handlers, notification_done(_)).Times(0);

    registrations.clear();

    client.request_set_user_brightness(10);
    client.request_user_auto_brightness_enable(true);
    client.request_set_inactivity_timeouts(1, 1);
    client.request_set_screen_power_mode("on", notification_reason);

    auto reply = client.request_keep_display_on();
    client.request_remove_display_on_request(reply.get());
}

TEST_F(AUnityScreenService, emits_display_power_state_change_signal)
{
    std::promise<rt::UnityScreenDBusClient::DisplayPowerStateChangeParams> promise;

    auto const reg = client.register_display_power_state_change_handler(
        [&promise] (auto v) { promise.set_value(v); });

    struct TestValues
    {
        repowerd::DisplayPowerChangeReason reason;
        repowerd::UnityScreenPowerStateChangeReason unity_screen_reason;
    };

    std::vector<TestValues> const test_values =
    {
        {repowerd::DisplayPowerChangeReason::activity,
         repowerd::UnityScreenPowerStateChangeReason::inactivity},
        {repowerd::DisplayPowerChangeReason::power_button,
         repowerd::UnityScreenPowerStateChangeReason::power_key},
        {repowerd::DisplayPowerChangeReason::proximity,
         repowerd::UnityScreenPowerStateChangeReason::proximity},
        {repowerd::DisplayPowerChangeReason::notification,
         repowerd::UnityScreenPowerStateChangeReason::notification},
        {repowerd::DisplayPowerChangeReason::call_done,
         repowerd::UnityScreenPowerStateChangeReason::call_done},
        {repowerd::DisplayPowerChangeReason::call,
         repowerd::UnityScreenPowerStateChangeReason::unknown},
    };

    for (auto const& v : test_values)
    {
        promise = {};
        auto future = promise.get_future();

        service.notify_display_power_on(v.reason);

        auto const params = future.get();
        EXPECT_THAT(params.power_state, Eq(1));
        EXPECT_THAT(params.reason, Eq(static_cast<int32_t>(v.unity_screen_reason)));
    }

    for (auto const& v : test_values)
    {
        promise = {};
        auto future = promise.get_future();

        service.notify_display_power_off(v.reason);

        auto const params = future.get();
        EXPECT_THAT(params.power_state, Eq(0));
        EXPECT_THAT(params.reason, Eq(static_cast<int32_t>(v.unity_screen_reason)));
    }
}

TEST_F(AUnityScreenService, logs_keep_display_on_request)
{
    auto const id = client.request_keep_display_on().get();

    EXPECT_TRUE(
        fake_log.contains_line(
            {"keepDisplayOn", client.unique_name(), std::to_string(id)}));
}

TEST_F(AUnityScreenService, logs_remove_display_on_request)
{
    auto const id = client.request_keep_display_on().get();
    client.request_remove_display_on_request(id).get();

    EXPECT_TRUE(
        fake_log.contains_line({
            "removeDisplayOnRequest",
            client.unique_name(),
            std::to_string(id)}));
}

TEST_F(AUnityScreenService, logs_user_auto_brightness_disable_request)
{
    client.request_user_auto_brightness_enable(false).get();

    EXPECT_TRUE(fake_log.contains_line({"userAutobrightnessEnable", "disable"}));
}

TEST_F(AUnityScreenService, logs_user_auto_brightness_enable_request)
{
    client.request_user_auto_brightness_enable(true).get();

    EXPECT_TRUE(fake_log.contains_line({"userAutobrightnessEnable", "enable"}));
}

TEST_F(AUnityScreenService, logs_set_user_brightness_request)
{
    int const brightness = 30;
    client.request_set_user_brightness(brightness).get();

    EXPECT_TRUE(fake_log.contains_line({"setUserBrightness", std::to_string(brightness)}));
}

TEST_F(AUnityScreenService, logs_set_inactivity_timeouts_request)
{
    int32_t const poweroff_timeout = 10;
    int32_t const dimmer_timeout = 5;

    client.request_set_inactivity_timeouts(poweroff_timeout, dimmer_timeout).get();

    EXPECT_TRUE(
        fake_log.contains_line({
            "setInactivityTimeouts",
            std::to_string(poweroff_timeout),
            std::to_string(dimmer_timeout)}));
}

TEST_F(AUnityScreenService, logs_set_screen_power_mode_request)
{
    client.request_set_screen_power_mode("on", notification_reason).get();

    EXPECT_TRUE(
        fake_log.contains_line({
            "setScreenPowerMode",
            client.unique_name(),
            "on",
            std::to_string(notification_reason)}));
}

TEST_F(AUnityScreenService, logs_emit_display_power_change_signal_for_on)
{
    service.notify_display_power_on(
        repowerd::DisplayPowerChangeReason::proximity);

    EXPECT_TRUE(
        fake_log.contains_line({
            "emit", "DisplayPowerStateChange",
            "1",
            std::to_string(
                static_cast<int>(
                    repowerd::UnityScreenPowerStateChangeReason::proximity))}));
}

TEST_F(AUnityScreenService, logs_emit_display_power_change_signal_for_off)
{
    service.notify_display_power_off(
        repowerd::DisplayPowerChangeReason::power_button);

    EXPECT_TRUE(
        fake_log.contains_line({
            "emit", "DisplayPowerStateChange",
            "0",
            std::to_string(
                static_cast<int>(
                    repowerd::UnityScreenPowerStateChangeReason::power_key))}));
}

TEST_F(AUnityScreenService, does_not_log_name_owner_changed_for_untracked_clients)
{
    client.disconnect();

    EXPECT_FALSE(fake_log.contains_line({"NameOwnerChanged"}));
}

TEST_F(AUnityScreenService, logs_name_owner_changed_for_tracked_keep_display_on_clients)
{
    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, enable_inactivity_timeout(_))
        .WillOnce(WakeUp(&request_processed));

    client.request_keep_display_on().get();
    client.disconnect();

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());

    EXPECT_TRUE(fake_log.contains_line({"NameOwnerChanged", client.unique_name()}));
}

TEST_F(AUnityScreenService, logs_name_owner_changed_for_tracked_notification_clients)
{
    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, notification_done(_))
        .WillOnce(WakeUp(&request_processed));

    client.request_set_screen_power_mode("on", notification_reason).get();
    client.disconnect();

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());

    EXPECT_TRUE(fake_log.contains_line({"NameOwnerChanged", client.unique_name()}));
}
