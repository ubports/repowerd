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

namespace rt = repowerd::test;

using namespace std::chrono_literals;

namespace
{

struct AClientRequest : rt::AcceptanceTest
{
    std::chrono::milliseconds const user_inactivity_normal_display_off_timeout{
        config.user_inactivity_normal_display_off_timeout()};
};

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

TEST_F(AClientRequest, to_enable_inactivity_timeout_turns_off_display_immediately_if_inactivity_timeout_has_expired)
{
    turn_on_display();

    expect_no_display_power_change();
    client_request_disable_inactivity_timeout();
    advance_time_by(user_inactivity_normal_display_off_timeout);
    verify_expectations();

    expect_display_turns_off();
    client_request_enable_inactivity_timeout();
}

TEST_F(AClientRequest,
       to_enable_inactivity_timeout_turns_off_display_after_existing_inactivity_timeout_expires)
{
    turn_on_display();

    expect_no_display_power_change();
    client_request_disable_inactivity_timeout();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);
    client_request_enable_inactivity_timeout();
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(1ms);
}
