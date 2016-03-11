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

#include "fake_client_requests.h"

#include <gtest/gtest.h>


namespace rt = repowerd::test;

using namespace std::chrono_literals;

namespace
{

struct AClientRequest : rt::AcceptanceTest
{
    void client_request_turn_on_display_with_normal_timeout()
    {
        config.the_fake_client_requests()->emit_turn_on_display(
            repowerd::TurnOnDisplayTimeout::normal);
    }

    void client_request_turn_on_display_with_reduced_timeout()
    {
        config.the_fake_client_requests()->emit_turn_on_display(
            repowerd::TurnOnDisplayTimeout::reduced);
    }

    std::chrono::milliseconds const user_inactivity_normal_display_off_timeout{
        config.user_inactivity_normal_display_off_timeout()};
    std::chrono::milliseconds const user_inactivity_reduced_display_off_timeout{
        config.user_inactivity_reduced_display_off_timeout()};
};

}

TEST_F(AClientRequest, to_turn_on_display_with_normal_timeout_works)
{
    expect_display_turns_on();
    client_request_turn_on_display_with_normal_timeout();
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(user_inactivity_normal_display_off_timeout);
}

TEST_F(AClientRequest, to_turn_on_display_with_normal_timeout_extends_existing_normal_timeout)
{
    turn_on_display();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);

    client_request_turn_on_display_with_normal_timeout();

    expect_no_display_power_change();
    advance_time_by(1ms);
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(user_inactivity_normal_display_off_timeout);
}

TEST_F(AClientRequest, to_turn_on_display_with_normal_timeout_extends_existing_reduced_timeout)
{
    client_request_turn_on_display_with_reduced_timeout();

    expect_no_display_power_change();
    advance_time_by(user_inactivity_reduced_display_off_timeout - 1ms);
    client_request_turn_on_display_with_normal_timeout();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(1ms);
}

TEST_F(AClientRequest, to_turn_on_display_with_normal_timeout_is_disallowed_by_proximity)
{
    set_proximity_state_near();

    expect_no_display_power_change();
    client_request_turn_on_display_with_normal_timeout();
}

TEST_F(AClientRequest, to_turn_on_display_with_reduced_timeout_works)
{
    expect_display_turns_on();
    client_request_turn_on_display_with_reduced_timeout();
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(user_inactivity_reduced_display_off_timeout);
}

TEST_F(AClientRequest, to_turn_on_display_with_reduced_timeout_extends_timeout)
{
    turn_on_display();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);

    client_request_turn_on_display_with_reduced_timeout();

    expect_no_display_power_change();
    advance_time_by(1ms);
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(user_inactivity_reduced_display_off_timeout);
}

TEST_F(AClientRequest,
       to_turn_on_display_with_reduced_timeout_does_not_reduce_existing_longer_timeout)
{
    turn_on_display();
    advance_time_by(
        user_inactivity_normal_display_off_timeout -
        user_inactivity_reduced_display_off_timeout -
        1ms);

    client_request_turn_on_display_with_reduced_timeout();

    expect_no_display_power_change();
    advance_time_by(user_inactivity_reduced_display_off_timeout);
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(1ms);
}

TEST_F(AClientRequest, to_turn_on_display_with_reduced_timeout_is_disallowed_by_proximity)
{
    set_proximity_state_near();

    expect_no_display_power_change();
    client_request_turn_on_display_with_reduced_timeout();
}
