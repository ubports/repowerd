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

#include "acceptance_test.h"

#include <gtest/gtest.h>

#include <chrono>

namespace rt = repowerd::test;

using namespace std::chrono_literals;

namespace
{

struct AClientRequest : rt::AcceptanceTest
{
    std::chrono::milliseconds const suspend_timeout{
        user_inactivity_normal_suspend_timeout + 10s};
};

}

TEST_F(AClientRequest, to_disable_inactivity_timeout_turns_on_display)
{
    expect_display_turns_on();
    client_request_disable_inactivity_timeout();
}

TEST_F(AClientRequest, to_disable_inactivity_timeout_brightens_display_if_already_on)
{
    turn_on_display();

    expect_display_brightens();
    client_request_disable_inactivity_timeout();
}

TEST_F(AClientRequest, to_disable_inactivity_timeout_works)
{
    turn_on_display();
    client_request_disable_inactivity_timeout();

    expect_no_display_power_change();
    advance_time_by(user_inactivity_normal_display_off_timeout);
}

TEST_F(AClientRequest, to_disable_inactivity_timeout_does_not_affect_power_button)
{
    turn_on_display();
    client_request_disable_inactivity_timeout();

    expect_display_turns_off();
    press_power_button();
    release_power_button();
}

TEST_F(AClientRequest, to_enable_inactivity_timeout_turns_off_display_after_normal_timeout_if_inactivity_timeout_has_expired)
{
    turn_on_display();

    expect_no_display_power_change();
    client_request_disable_inactivity_timeout();
    advance_time_by(user_inactivity_normal_display_off_timeout);
    verify_expectations();

    expect_no_display_power_change();
    client_request_enable_inactivity_timeout();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(1ms);
}

TEST_F(AClientRequest,
       to_enable_inactivity_timeout_turns_off_display_extends_existing_inactivity_timeout)
{
    turn_on_display();

    expect_no_display_power_change();
    client_request_disable_inactivity_timeout();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);
    client_request_enable_inactivity_timeout();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(1ms);
}

TEST_F(AClientRequest, to_set_inactivity_timeout_affects_display_off_timeout)
{
    auto const display_off_timeout = user_inactivity_normal_display_off_timeout + 10s;

    client_request_set_inactivity_timeout(display_off_timeout);

    turn_on_display();

    expect_no_display_power_change();
    advance_time_by(display_off_timeout - 1ms);
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(1ms);
}

TEST_F(AClientRequest, to_set_inactivity_timeout_affects_display_dim_timeout)
{
    auto const display_off_timeout = user_inactivity_normal_display_off_timeout + 10s;
    auto const display_dim_timeout =
        display_off_timeout - user_inactivity_normal_display_dim_duration;

    client_request_set_inactivity_timeout(display_off_timeout);

    turn_on_display();

    expect_no_display_brightness_change();
    advance_time_by(display_dim_timeout - 1ms);
    verify_expectations();

    expect_display_dims();
    advance_time_by(1ms);
}

TEST_F(AClientRequest, to_set_short_inactivity_timeout_cancels_display_dim_timeout)
{
    auto const display_off_timeout = user_inactivity_normal_display_dim_duration;

    client_request_set_inactivity_timeout(display_off_timeout);

    turn_on_display();

    expect_no_display_brightness_change();
    advance_time_by(display_off_timeout - 1ms);
}

TEST_F(AClientRequest,
       to_set_infinite_inactivity_timeout_keeps_screen_on_forever_if_turned_on_by_power_button)
{
    turn_on_display();

    client_request_set_inactivity_timeout(infinite_timeout);

    expect_no_display_power_change();
    expect_no_display_brightness_change();
    advance_time_by(1h);
}

TEST_F(AClientRequest,
       to_set_infinite_inactivity_timeout_keeps_screen_on_forever_after_user_activity)
{
    expect_display_turns_on();
    emit_notification();
    emit_notification_done();
    verify_expectations();

    perform_user_activity_extending_power_state();
    client_request_set_inactivity_timeout(infinite_timeout);

    expect_no_display_power_change();
    expect_no_display_brightness_change();
    advance_time_by(1h);
}

TEST_F(AClientRequest,
       to_set_infinite_inactivity_allows_power_button_to_turn_off_screen)
{
    turn_on_display();
    client_request_set_inactivity_timeout(infinite_timeout);
    turn_off_display();
}

TEST_F(AClientRequest, to_set_non_positive_inactivity_timeout_is_ignrored)
{
    client_request_set_inactivity_timeout(-1ms);
    client_request_set_inactivity_timeout(0ms);

    turn_on_display();

    expect_no_display_power_change();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(1ms);
}

TEST_F(AClientRequest, to_disable_inactivity_timeout_works_until_all_requests_are_removed)
{
    std::string const inactivity_id1{"1"};
    std::string const inactivity_id2{"2"};
    std::string const inactivity_id3{"3"};

    expect_display_turns_on();
    client_request_disable_inactivity_timeout(inactivity_id1);
    verify_expectations();

    expect_display_brightens();
    client_request_disable_inactivity_timeout(inactivity_id2);
    verify_expectations();

    expect_display_brightens();
    client_request_disable_inactivity_timeout(inactivity_id3);
    verify_expectations();

    expect_no_display_power_change();
    client_request_enable_inactivity_timeout(inactivity_id1);
    advance_time_by(user_inactivity_normal_display_off_timeout);
    verify_expectations();

    expect_no_display_power_change();
    client_request_enable_inactivity_timeout(inactivity_id2);
    advance_time_by(user_inactivity_normal_display_off_timeout);
    verify_expectations();

    expect_display_turns_off();
    client_request_enable_inactivity_timeout(inactivity_id3);
    advance_time_by(user_inactivity_normal_display_off_timeout);
    verify_expectations();
}

TEST_F(AClientRequest, to_enable_autobrightness_works)
{
    expect_autobrightness_enabled();

    client_request_enable_autobrightness();
}

TEST_F(AClientRequest, to_disable_autobrightness_works)
{
    expect_autobrightness_disabled();

    client_request_disable_autobrightness();
}

TEST_F(AClientRequest, to_set_brightness_value_works)
{
    expect_normal_brightness_value_set_to(0.56);

    client_request_set_normal_brightness_value(0.56);
}

TEST_F(AClientRequest, to_disallow_suspend_works)
{
    turn_on_display();

    client_setting_set_inactivity_behavior(
        repowerd::PowerAction::suspend,
        repowerd::PowerSupply::battery,
        suspend_timeout);

    client_request_disallow_suspend();

    expect_display_turns_off();
    expect_no_system_power_change();
    advance_time_by(suspend_timeout);
}

TEST_F(AClientRequest, to_allow_suspend_suspends_immediately_if_timeout_expired)
{
    turn_on_display();

    client_setting_set_inactivity_behavior(
        repowerd::PowerAction::suspend,
        repowerd::PowerSupply::battery,
        suspend_timeout);

    client_request_disallow_suspend();

    expect_display_turns_off();
    expect_no_system_power_change();
    advance_time_by(suspend_timeout);
    verify_expectations();

    expect_system_suspends();
    client_request_allow_suspend();
}

TEST_F(AClientRequest, to_allow_suspend_does_not_suspend_immediately_if_timeout_not_expired)
{
    turn_on_display();

    client_setting_set_inactivity_behavior(
        repowerd::PowerAction::suspend,
        repowerd::PowerSupply::battery,
        suspend_timeout);

    client_request_disallow_suspend();

    expect_display_turns_off();
    advance_time_by(user_inactivity_normal_suspend_timeout);
    verify_expectations();

    expect_no_system_power_change();
    client_request_allow_suspend();
    verify_expectations();

    expect_system_suspends();
    advance_time_by(10s);
}

TEST_F(AClientRequest, to_disable_inactivity_timeout_is_logged)
{
    client_request_disable_inactivity_timeout();

    EXPECT_TRUE(log_contains_line({"disable_inactivity_timeout"}));
}

TEST_F(AClientRequest, to_enable_inactivity_timeout_is_logged)
{
    client_request_enable_inactivity_timeout();

    EXPECT_TRUE(log_contains_line({"enable_inactivity_timeout"}));
}

TEST_F(AClientRequest, to_set_inactivity_timeout_is_logged)
{
    client_request_set_inactivity_timeout(15000ms);

    EXPECT_TRUE(log_contains_line({"set_inactivity_behavior", "display_off", "battery", "15000"}));
    EXPECT_TRUE(log_contains_line({"set_inactivity_behavior", "display_off", "line_power", "15000"}));
}

TEST_F(AClientRequest, to_disable_autobrightness_is_logged)
{
    client_request_disable_autobrightness();

    EXPECT_TRUE(log_contains_line({"disable_autobrightness"}));
}

TEST_F(AClientRequest, to_enable_autobrightness_is_logged)
{
    client_request_enable_autobrightness();

    EXPECT_TRUE(log_contains_line({"enable_autobrightness"}));
}

TEST_F(AClientRequest, to_set_normal_brightness_value_is_logged)
{
    client_request_set_normal_brightness_value(0.67);

    EXPECT_TRUE(log_contains_line({"set_normal_brightness_value", "0.67"}));
}

TEST_F(AClientRequest, to_allow_suspend_is_logged)
{
    client_request_allow_suspend();

    EXPECT_TRUE(log_contains_line({"allow_suspend"}));
}

TEST_F(AClientRequest, to_disallow_suspend_is_logged)
{
    client_request_disallow_suspend();

    EXPECT_TRUE(log_contains_line({"disallow_suspend"}));
}
