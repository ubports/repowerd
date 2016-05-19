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

#include "src/adapters/ofono_voice_call_service.h"

#include "dbus_bus.h"
#include "fake_log.h"
#include "fake_ofono.h"
#include "fake_shared.h"
#include "spin_wait.h"
#include "wait_condition.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>
#include <algorithm>

namespace rt = repowerd::test;
using namespace std::chrono_literals;
using namespace testing;

namespace
{

auto modems_match_condition(rt::FakeOfono::Modems const& match)
{
    return
        [match](rt::FakeOfono::Modems const& modems)
        {
            return match == modems;
        };
}

auto modems_with_power(
    std::vector<std::string> const& modems,
    rt::FakeOfono::ModemPowerState state)
{
    rt::FakeOfono::Modems match;
    for (auto const& modem : modems)
        match[modem] = state;
    return modems_match_condition(match);
}

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

        ofono.add_modem(initial_modem);

        ofono_voice_call_service.start_processing();
    }

    std::string call_path(int i)
    {
        return "/phonesim/call0" + std::to_string(i);
    }

    void wait_for_tracked_modems(std::unordered_set<std::string> const& modems)
    {
        auto const result = rt::spin_wait_for_condition_or_timeout(
            [this,&modems] { return ofono_voice_call_service.tracked_modems() == modems; },
            default_timeout);
        if (!result)
            throw std::runtime_error("Timeout while waiting for tracked modems");
    }

    struct MockHandlers
    {
        MOCK_METHOD0(active_call, void());
        MOCK_METHOD0(no_active_call, void());
    };
    testing::NiceMock<MockHandlers> mock_handlers;

    rt::DBusBus bus;
    rt::FakeLog fake_log;
    repowerd::OfonoVoiceCallService ofono_voice_call_service{
        rt::fake_shared(fake_log),
        bus.address()};
    rt::FakeOfono ofono{bus.address()};

    std::vector<repowerd::HandlerRegistration> registrations;
    std::chrono::seconds const default_timeout{3};

    std::string const initial_modem{"/phonesim"};
    std::string const modem1{"/modem01"};
    std::string const modem2{"/modem02"};
};

}

TEST_F(AnOfonoVoiceCallService, calls_handler_for_new_active_calls)
{
    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, active_call())
        .WillOnce(Return())
        .WillOnce(Return())
        .WillOnce(WakeUp(&request_processed));

    ofono.add_call(call_path(1), repowerd::OfonoCallState::active);
    ofono.add_call(call_path(2), repowerd::OfonoCallState::alerting);
    ofono.add_call(call_path(3), repowerd::OfonoCallState::dialing);

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());
}

TEST_F(AnOfonoVoiceCallService, does_not_call_handler_for_new_inactive_calls)
{
    EXPECT_CALL(mock_handlers, active_call()).Times(0);

    ofono.add_call(call_path(1), repowerd::OfonoCallState::disconnected);
    ofono.add_call(call_path(2), repowerd::OfonoCallState::held);
    ofono.add_call(call_path(3), repowerd::OfonoCallState::incoming);
    ofono.add_call(call_path(4), repowerd::OfonoCallState::waiting);

    // Give some time to requests to be processed
    std::this_thread::sleep_for(100ms);
}

TEST_F(AnOfonoVoiceCallService, calls_handler_for_no_active_call_when_call_removed)
{
    rt::WaitCondition request_processed;

    ofono.add_call(call_path(1), repowerd::OfonoCallState::active);
    ofono.add_call(call_path(2), repowerd::OfonoCallState::active);
    ofono.add_call(call_path(3), repowerd::OfonoCallState::alerting);

    EXPECT_CALL(mock_handlers, no_active_call()).Times(0);

    ofono.remove_call(call_path(1));
    ofono.remove_call(call_path(3));
    // Give some time to requests to be processed
    std::this_thread::sleep_for(100ms);

    Mock::VerifyAndClearExpectations(&mock_handlers);

    EXPECT_CALL(mock_handlers, no_active_call())
        .WillOnce(WakeUp(&request_processed));

    ofono.remove_call(call_path(2));

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());
}

TEST_F(AnOfonoVoiceCallService, calls_handler_for_active_call_when_call_state_changed)
{
    ofono.add_call(call_path(1), repowerd::OfonoCallState::incoming);
    ofono.add_call(call_path(2), repowerd::OfonoCallState::held);

    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, active_call())
        .WillOnce(Return())
        .WillOnce(WakeUp(&request_processed));

    ofono.change_call_state(call_path(1), repowerd::OfonoCallState::active);
    ofono.change_call_state(call_path(2), repowerd::OfonoCallState::active);

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());
}

TEST_F(AnOfonoVoiceCallService, calls_handler_for_no_active_call_when_call_state_changed)
{
    ofono.add_call(call_path(1), repowerd::OfonoCallState::active);
    ofono.add_call(call_path(2), repowerd::OfonoCallState::active);

    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, no_active_call())
        .WillOnce(WakeUp(&request_processed));

    ofono.change_call_state(call_path(1), repowerd::OfonoCallState::held);
    ofono.change_call_state(call_path(2), repowerd::OfonoCallState::held);

    request_processed.wait_for(default_timeout);
    EXPECT_TRUE(request_processed.woken());
}

TEST_F(AnOfonoVoiceCallService, sets_power_mode_on_initial_modems)
{
    ofono_voice_call_service.set_low_power_mode();

    auto condition = ofono.wait_for_modems_condition(
        modems_with_power(
            {initial_modem},
            rt::FakeOfono::ModemPowerState::low),
        default_timeout);

    EXPECT_TRUE(condition);

    ofono_voice_call_service.set_normal_power_mode();

    condition = ofono.wait_for_modems_condition(
        modems_with_power(
            {initial_modem},
            rt::FakeOfono::ModemPowerState::normal),
        default_timeout);

    EXPECT_TRUE(condition);
}

TEST_F(AnOfonoVoiceCallService, sets_power_mode_on_modems_added_later)
{
    ofono.add_modem(modem1);
    ofono.add_modem(modem2);
    auto all_modems = {initial_modem, modem1, modem2};

    wait_for_tracked_modems(all_modems);
    ofono_voice_call_service.set_low_power_mode();

    auto condition = ofono.wait_for_modems_condition(
        modems_with_power(
            all_modems,
            rt::FakeOfono::ModemPowerState::low),
        default_timeout);

    EXPECT_TRUE(condition);

    ofono_voice_call_service.set_normal_power_mode();

    condition = ofono.wait_for_modems_condition(
        modems_with_power(
            all_modems,
            rt::FakeOfono::ModemPowerState::normal),
        default_timeout);

    EXPECT_TRUE(condition);
}

TEST_F(AnOfonoVoiceCallService, does_not_set_power_mode_on_removed_modems)
{
    ofono.add_modem(modem1);
    ofono.add_modem(modem2);
    auto all_modems = {initial_modem, modem1, modem2};
    auto all_modems_but_modem1 = {initial_modem, modem2};

    wait_for_tracked_modems(all_modems);
    ofono.remove_modem(modem1);
    wait_for_tracked_modems(all_modems_but_modem1);

    ofono_voice_call_service.set_low_power_mode();

    auto const condition = ofono.wait_for_modems_condition(
        modems_with_power(
            all_modems_but_modem1,
            rt::FakeOfono::ModemPowerState::low),
        default_timeout);

    EXPECT_TRUE(condition);
}

TEST_F(AnOfonoVoiceCallService, logs_call_added)
{
    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, active_call())
        .WillOnce(Return())
        .WillOnce(Return())
        .WillOnce(WakeUp(&request_processed));

    ofono.add_call(call_path(1), repowerd::OfonoCallState::active);
    ofono.add_call(call_path(2), repowerd::OfonoCallState::alerting);
    ofono.add_call(call_path(3), repowerd::OfonoCallState::dialing);

    request_processed.wait_for(default_timeout);

    EXPECT_TRUE(fake_log.contains_line({"CallAdded", call_path(1), "active"}));
    EXPECT_TRUE(fake_log.contains_line({"CallAdded", call_path(2), "alerting"}));
    EXPECT_TRUE(fake_log.contains_line({"CallAdded", call_path(3), "dialing"}));
}

TEST_F(AnOfonoVoiceCallService, logs_call_removed)
{
    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, active_call());
    EXPECT_CALL(mock_handlers, no_active_call())
        .WillOnce(WakeUp(&request_processed));

    ofono.add_call(call_path(1), repowerd::OfonoCallState::active);
    ofono.remove_call(call_path(1));

    request_processed.wait_for(default_timeout);

    EXPECT_TRUE(fake_log.contains_line({"CallRemoved", call_path(1)}));
}

TEST_F(AnOfonoVoiceCallService, logs_call_state_changed)
{
    ofono.add_call(call_path(1), repowerd::OfonoCallState::incoming);

    rt::WaitCondition request_processed;

    EXPECT_CALL(mock_handlers, active_call())
        .WillOnce(WakeUp(&request_processed));

    ofono.change_call_state(call_path(1), repowerd::OfonoCallState::active);

    request_processed.wait_for(default_timeout);

    EXPECT_TRUE(fake_log.contains_line({"CallStateChanged", call_path(1), "active"}));
}

TEST_F(AnOfonoVoiceCallService, logs_existing_modems)
{
    EXPECT_TRUE(fake_log.contains_line({"add_existing_modems", initial_modem}));
}

TEST_F(AnOfonoVoiceCallService, logs_modem_added)
{
    ofono.add_modem(modem1);
    ofono.add_modem(modem2);
    auto all_modems = {initial_modem, modem1, modem2};

    wait_for_tracked_modems(all_modems);

    EXPECT_TRUE(fake_log.contains_line({"ModemAdded", modem1}));
    EXPECT_TRUE(fake_log.contains_line({"ModemAdded", modem2}));
}

TEST_F(AnOfonoVoiceCallService, logs_modem_removed)
{
    ofono.add_modem(modem1);
    ofono.add_modem(modem2);
    auto all_modems = {initial_modem, modem1, modem2};
    auto all_modems_but_modem1 = {initial_modem, modem2};

    wait_for_tracked_modems(all_modems);
    ofono.remove_modem(modem1);
    wait_for_tracked_modems(all_modems_but_modem1);

    EXPECT_TRUE(fake_log.contains_line({"ModemRemoved", modem1}));
}

TEST_F(AnOfonoVoiceCallService, logs_set_fast_dormancy)
{
    ofono.add_modem(modem1);
    auto all_modems = {initial_modem, modem1};

    wait_for_tracked_modems(all_modems);

    ofono_voice_call_service.set_low_power_mode();

    ofono.wait_for_modems_condition(
        modems_with_power(
            all_modems,
            rt::FakeOfono::ModemPowerState::low),
        default_timeout);

    for (auto const& modem : all_modems)
        EXPECT_TRUE(fake_log.contains_line({"set_fast_dormancy", modem, "true"}));

    ofono_voice_call_service.set_normal_power_mode();

    ofono.wait_for_modems_condition(
        modems_with_power(
            all_modems,
            rt::FakeOfono::ModemPowerState::normal),
        default_timeout);

    for (auto const& modem : all_modems)
        EXPECT_TRUE(fake_log.contains_line({"set_fast_dormancy", modem, "false"}));
}
