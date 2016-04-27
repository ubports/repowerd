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
#include "src/adapters/ofono_voice_call_service.h"

#include "wait_condition.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>

namespace rt = repowerd::test;
using namespace std::chrono_literals;
using namespace testing;

namespace
{

std::string ofono_call_state_to_string(repowerd::OfonoCallState state)
{
    switch (state)
    {
    case repowerd::OfonoCallState::active:
        return "active";
    case repowerd::OfonoCallState::alerting:
        return "alerting";
    case repowerd::OfonoCallState::dialing:
        return "dialing";
    case repowerd::OfonoCallState::disconnected:
        return "disconnected";
    case repowerd::OfonoCallState::held:
        return "held";
    case repowerd::OfonoCallState::incoming:
        return "incoming";
    case repowerd::OfonoCallState::waiting:
        return "waiting";
    case repowerd::OfonoCallState::invalid:
    default:
        return "";
    };

    return "";
}

struct OfonoDBusClient : rt::DBusClient
{
    OfonoDBusClient(std::string const& dbus_address)
        : rt::DBusClient{
            dbus_address,
            "org.ofono",
            "/phonesim"}
    {
        connection.request_name("org.ofono");
    }

    void emit_call_added(std::string const& call_path, repowerd::OfonoCallState call_state)
    {
        auto const params = g_variant_new_parsed(
            "(@o %o, @a{sv} {'State': <%s>, 'Name': <'bla'>})",
            call_path.c_str(), ofono_call_state_to_string(call_state).c_str());
        emit_signal("org.ofono.VoiceCallManager", "CallAdded", params);
    }

    void emit_call_removed(std::string const& call_path)
    {
        auto const params = g_variant_new_parsed("(@o %o,)", call_path.c_str());
        emit_signal("org.ofono.VoiceCallManager", "CallRemoved", params);
    }

    void emit_call_state_change(std::string const& call_path, repowerd::OfonoCallState call_state)
    {
        auto const params = g_variant_new_parsed(
            "('State',<%s>)",
            ofono_call_state_to_string(call_state).c_str());

        emit_signal_full(call_path.c_str(), "org.ofono.VoiceCall", "PropertyChanged", params);
    }
};

struct AnOfonoVoiceCallService : testing::Test
{
    AnOfonoVoiceCallService()
    {
        registrations.push_back(
            ofono_voice_call_service.register_active_call_handler(
                [this] { mock_handlers.active_call(); }));
        registrations.push_back(
            ofono_voice_call_service.register_no_active_call_handler(
                [this] { mock_handlers.no_active_call(); }));

        ofono_voice_call_service.start_processing();
    }

    std::string call_path(int i)
    {
        return "/phonesim/call0" + std::to_string(i);
    }

    struct MockHandlers
    {
        MOCK_METHOD0(active_call, void());
        MOCK_METHOD0(no_active_call, void());
    };
    testing::NiceMock<MockHandlers> mock_handlers;

    rt::DBusBus bus;
    repowerd::OfonoVoiceCallService ofono_voice_call_service{bus.address()};
    OfonoDBusClient client{bus.address()};

    std::vector<repowerd::HandlerRegistration> registrations;
    std::chrono::seconds const default_timeout{3};
};

}

TEST_F(AnOfonoVoiceCallService, calls_handler_for_new_active_calls)
{
    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, active_call())
        .WillOnce(Return())
        .WillOnce(Return())
        .WillOnce(WakeUp(&request_processed));

    client.emit_call_added(call_path(1), repowerd::OfonoCallState::active);
    client.emit_call_added(call_path(2), repowerd::OfonoCallState::alerting);
    client.emit_call_added(call_path(3), repowerd::OfonoCallState::dialing);

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());
}

TEST_F(AnOfonoVoiceCallService, does_not_call_handler_for_new_inactive_calls)
{
    EXPECT_CALL(mock_handlers, active_call()).Times(0);

    client.emit_call_added(call_path(1), repowerd::OfonoCallState::disconnected);
    client.emit_call_added(call_path(2), repowerd::OfonoCallState::held);
    client.emit_call_added(call_path(3), repowerd::OfonoCallState::incoming);
    client.emit_call_added(call_path(4), repowerd::OfonoCallState::waiting);

    // Give some time to requests to be processed
    std::this_thread::sleep_for(100ms);
}

TEST_F(AnOfonoVoiceCallService, calls_handler_for_no_active_call_when_call_removed)
{
    rt::WaitCondition request_processed;

    client.emit_call_added(call_path(1), repowerd::OfonoCallState::active);
    client.emit_call_added(call_path(2), repowerd::OfonoCallState::active);
    client.emit_call_added(call_path(3), repowerd::OfonoCallState::alerting);

    EXPECT_CALL(mock_handlers, no_active_call()).Times(0);

    client.emit_call_removed(call_path(1));
    client.emit_call_removed(call_path(3));
    // Give some time to requests to be processed
    std::this_thread::sleep_for(100ms);

    Mock::VerifyAndClearExpectations(&mock_handlers);

    EXPECT_CALL(mock_handlers, no_active_call())
        .WillOnce(WakeUp(&request_processed));

    client.emit_call_removed(call_path(2));

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());
}

TEST_F(AnOfonoVoiceCallService, calls_handler_for_active_call_when_call_state_changed)
{
    client.emit_call_added(call_path(1), repowerd::OfonoCallState::incoming);
    client.emit_call_added(call_path(2), repowerd::OfonoCallState::held);

    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, active_call())
        .WillOnce(Return())
        .WillOnce(WakeUp(&request_processed));

    client.emit_call_state_change(call_path(1), repowerd::OfonoCallState::active);
    client.emit_call_state_change(call_path(2), repowerd::OfonoCallState::active);

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());
}

TEST_F(AnOfonoVoiceCallService, calls_handler_for_no_active_call_when_call_state_changed)
{
    client.emit_call_added(call_path(1), repowerd::OfonoCallState::active);
    client.emit_call_added(call_path(2), repowerd::OfonoCallState::active);

    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, no_active_call())
        .WillOnce(WakeUp(&request_processed));

    client.emit_call_state_change(call_path(1), repowerd::OfonoCallState::held);
    client.emit_call_state_change(call_path(2), repowerd::OfonoCallState::held);

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());
}
