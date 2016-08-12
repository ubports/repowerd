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
    emit_no_notification();
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

    EXPECT_TRUE(log_contains_line({"set_inactivity_timeout", "15000"}));
}
