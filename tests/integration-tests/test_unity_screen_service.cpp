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
#include "unity_screen_dbus_client.h"
#include "src/adapters/dbus_connection_handle.h"
#include "src/adapters/dbus_message_handle.h"
#include "src/adapters/unity_screen_service.h"

#include "wait_condition.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>

namespace rt = repowerd::test;

namespace
{

struct AUnityScreenService : testing::Test
{
    AUnityScreenService()
    {
        service.register_disable_inactivity_timeout_handler(
            [this] { mock_handlers.disable_inactivity_timeout(); });
        service.register_enable_inactivity_timeout_handler(
            [this] { mock_handlers.enable_inactivity_timeout(); });
        service.register_set_inactivity_timeout_handler(
            [this] (std::chrono::milliseconds ms)
            {
                mock_handlers.set_inactivity_timeout(ms);
            });

        service.register_disable_autobrightness_handler(
            [this] { mock_handlers.disable_autobrightness(); });
        service.register_enable_autobrightness_handler(
            [this] { mock_handlers.enable_autobrightness(); });
        service.register_set_normal_brightness_value_handler(
            [this] (float v)
            {
                mock_handlers.set_normal_brightness_value(v);
            });
    }

    struct MockHandlers
    {
        MOCK_METHOD0(disable_inactivity_timeout, void());
        MOCK_METHOD0(enable_inactivity_timeout, void());
        MOCK_METHOD1(set_inactivity_timeout, void(std::chrono::milliseconds));

        MOCK_METHOD0(disable_autobrightness, void());
        MOCK_METHOD0(enable_autobrightness, void());
        MOCK_METHOD1(set_normal_brightness_value, void(float));
    };
    testing::NiceMock<MockHandlers> mock_handlers;

    rt::DBusBus bus;
    repowerd::UnityScreenService service{bus.address()};
    rt::UnityScreenDBusClient client{bus.address()};
    std::chrono::seconds const default_timeout{3};
};

}

TEST_F(AUnityScreenService, replies_to_introspection_request)
{
    using namespace testing;
    auto reply = client.request_introspection();
    EXPECT_THAT(reply.get(), StrNe(""));
}

TEST_F(AUnityScreenService, forwards_keep_display_on_request)
{
    EXPECT_CALL(mock_handlers, disable_inactivity_timeout());

    client.request_keep_display_on();
}

TEST_F(AUnityScreenService, replies_with_different_ids_to_keep_display_on_requests)
{
    using namespace testing;

    auto reply1 = client.request_keep_display_on();
    auto reply2 = client.request_keep_display_on();

    auto const id1 = reply1.get();
    auto const id2 = reply2.get();

    EXPECT_THAT(id1, Ne(id2));
}

TEST_F(AUnityScreenService, disables_keep_display_on_when_single_request_is_removed)
{
    using namespace testing;

    InSequence s;
    EXPECT_CALL(mock_handlers, disable_inactivity_timeout());
    EXPECT_CALL(mock_handlers, enable_inactivity_timeout());

    auto reply1 = client.request_keep_display_on();
    client.request_remove_display_on_request(reply1.get());
}

TEST_F(AUnityScreenService, disables_keep_display_on_when_all_requests_are_removed)
{
    using namespace testing;

    EXPECT_CALL(mock_handlers, disable_inactivity_timeout()).Times(3);
    EXPECT_CALL(mock_handlers, enable_inactivity_timeout()).Times(0);

    auto reply1 = client.request_keep_display_on();
    auto reply2 = client.request_keep_display_on();
    auto reply3 = client.request_keep_display_on();

    client.request_remove_display_on_request(reply1.get());
    client.request_remove_display_on_request(reply2.get());
    auto id3 = reply3.get();

    // Display should still be kept on at this point
    Mock::VerifyAndClearExpectations(&mock_handlers);

    // keep_display_on should be disable only when the last request is removed
    EXPECT_CALL(mock_handlers, enable_inactivity_timeout());

    client.request_remove_display_on_request(id3);
}

TEST_F(AUnityScreenService, disables_keep_display_on_when_single_client_disconnects)
{
    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, disable_inactivity_timeout()).Times(3);
    EXPECT_CALL(mock_handlers, enable_inactivity_timeout())
        .WillOnce(WakeUp(&request_processed));

    client.request_keep_display_on();
    client.request_keep_display_on();
    client.request_keep_display_on();

    client.disconnect();

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());
}

TEST_F(AUnityScreenService, disables_keep_display_on_when_all_clients_disconnect_or_remove_requests)
{
    using namespace testing;

    rt::UnityScreenDBusClient other_client{bus.address()};

    EXPECT_CALL(mock_handlers, disable_inactivity_timeout()).Times(4);
    EXPECT_CALL(mock_handlers, enable_inactivity_timeout()).Times(0);

    auto reply1 = client.request_keep_display_on();
    auto reply2 = client.request_keep_display_on();
    other_client.request_keep_display_on();
    other_client.request_keep_display_on();

    other_client.disconnect();
    client.request_remove_display_on_request(reply1.get());
    auto id2 = reply2.get();

    // Display should still be kept on at this point
    Mock::VerifyAndClearExpectations(&mock_handlers);

    // keep_display_on should be disabled only when the last request is removed
    rt::WaitCondition request_processed;
    EXPECT_CALL(mock_handlers, enable_inactivity_timeout())
        .WillOnce(WakeUp(&request_processed));

    client.request_remove_display_on_request(id2);

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());
}

TEST_F(AUnityScreenService, ignores_invalid_display_on_removal_request)
{
    int32_t const invalid_id{-1};
    EXPECT_CALL(mock_handlers, enable_inactivity_timeout()).Times(0);

    client.request_remove_display_on_request(invalid_id);
    client.disconnect();

    // Allow some time for dbus calls to reach UnityScreenService
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(AUnityScreenService, ignores_disconnects_from_clients_without_display_on_request)
{
    EXPECT_CALL(mock_handlers, enable_inactivity_timeout()).Times(0);

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
    float const brightness_normalized = brightness/255.0f;

    EXPECT_CALL(mock_handlers, set_normal_brightness_value(brightness_normalized));
    client.request_set_user_brightness(brightness);
}

TEST_F(AUnityScreenService, returns_error_reply_for_invalid_method)
{
    using namespace testing;

    auto reply = client.request_invalid_method();
    auto reply_msg = reply.get();

    EXPECT_THAT(g_dbus_message_get_message_type(reply_msg), Eq(G_DBUS_MESSAGE_TYPE_ERROR));
}

TEST_F(AUnityScreenService, returns_error_reply_for_method_with_invalid_interface)
{
    using namespace testing;

    auto reply = client.request_method_with_invalid_interface();
    auto reply_msg = reply.get();

    EXPECT_THAT(g_dbus_message_get_message_type(reply_msg), Eq(G_DBUS_MESSAGE_TYPE_ERROR));
}

TEST_F(AUnityScreenService, returns_error_reply_for_method_with_invalid_arguments)
{
    using namespace testing;

    auto reply = client.request_method_with_invalid_arguments();
    auto reply_msg = reply.get();

    EXPECT_THAT(g_dbus_message_get_message_type(reply_msg), Eq(G_DBUS_MESSAGE_TYPE_ERROR));
}

TEST_F(AUnityScreenService, returns_error_reply_for_set_touch_visualization_enabled_request)
{
    using namespace testing;

    auto reply = client.request_set_touch_visualization_enabled(true);
    auto reply_msg = static_cast<rt::DBusAsyncReply*>(&reply)->get();

    EXPECT_THAT(g_dbus_message_get_message_type(reply_msg), Eq(G_DBUS_MESSAGE_TYPE_ERROR));
}
