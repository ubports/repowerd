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

struct APowerSource : rt::AcceptanceTest
{
};

}

TEST_F(APowerSource, change_turns_on_display_for_reduced_timeout)
{
    expect_display_turns_on();
    emit_power_source_change();
    verify_expectations();

    expect_no_display_power_change();
    advance_time_by(user_inactivity_reduced_display_off_timeout - 1ms);
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(1ms);
}

TEST_F(APowerSource, change_brightens_display_if_it_is_already_on)
{
    turn_on_display();

    expect_display_dims();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);
    verify_expectations();

    expect_no_display_power_change();
    expect_display_brightens();
    emit_power_source_change();
}

TEST_F(APowerSource, change_reschedules_timeout_if_display_is_already_on)
{
    turn_on_display();

    expect_no_display_power_change();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);
    emit_power_source_change();
    verify_expectations();

    expect_no_display_power_change();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(1ms);
}

TEST_F(APowerSource, change_does_not_turn_off_display_if_inactivity_timeout_is_disabled_and_display_was_on)
{
    client_request_disable_inactivity_timeout();

    expect_no_display_power_change();
    advance_time_by(user_inactivity_normal_display_off_timeout);
    emit_power_source_change();
    advance_time_by(user_inactivity_reduced_display_off_timeout);
    verify_expectations();
}

TEST_F(APowerSource, change_turns_off_display_if_inactivity_timeout_is_disabled_and_display_was_off)
{
    client_request_disable_inactivity_timeout();
    turn_off_display();

    expect_display_turns_on();
    emit_power_source_change();
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(user_inactivity_reduced_display_off_timeout);
}

TEST_F(APowerSource, critical_state_powers_off_system)
{
    expect_system_powers_off();

    emit_power_source_critical();
}

TEST_F(APowerSource, change_is_logged)
{
    emit_power_source_change();

    EXPECT_TRUE(log_contains_line({"power_source_change"}));
}

TEST_F(APowerSource, critical_state_is_logged)
{
    emit_power_source_critical();

    EXPECT_TRUE(log_contains_line({"power_source_critical"}));
}
