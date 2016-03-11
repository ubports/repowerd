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

#include "fake_user_activity.h"

#include <gtest/gtest.h>

#include <chrono>

namespace rt = repowerd::test;

using namespace std::chrono_literals;

namespace
{

struct AUserActivity : rt::AcceptanceTest
{
    std::chrono::milliseconds const user_inactivity_normal_display_off_timeout{
        config.user_inactivity_normal_display_off_timeout()};
};

}

TEST_F(AUserActivity, not_performed_turns_off_display_after_timeout)
{
    turn_on_display();

    expect_display_turns_off();
    advance_time_by(user_inactivity_normal_display_off_timeout);
}

TEST_F(AUserActivity, not_performed_does_not_turn_off_display_prematurely)
{
    turn_on_display();

    expect_no_display_power_change();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);
}

TEST_F(AUserActivity, not_performed_has_no_effect_after_display_is_turned_off)
{
    turn_on_display();
    turn_off_display();

    expect_no_display_power_change();
    advance_time_by(user_inactivity_normal_display_off_timeout);
}

TEST_F(AUserActivity, extending_power_state_has_no_effect_when_display_is_off)
{
    expect_no_display_power_change();

    perform_user_activity_extending_power_state();
    advance_time_by(user_inactivity_normal_display_off_timeout);
}

TEST_F(AUserActivity, extending_power_state_resets_display_off_timer)
{
    turn_on_display();

    expect_no_display_power_change();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);
    perform_user_activity_extending_power_state();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(1ms);
}

TEST_F(AUserActivity, changing_power_state_turns_on_display_immediately)
{
    expect_display_turns_on();

    perform_user_activity_changing_power_state();
}

TEST_F(AUserActivity, changing_power_state_does_not_turn_on_display_if_it_is_already_on)
{
    turn_on_display();

    expect_no_display_power_change();
    perform_user_activity_changing_power_state();
}

TEST_F(AUserActivity, changing_power_state_resets_display_off_timer)
{
    turn_on_display();

    expect_no_display_power_change();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);
    perform_user_activity_changing_power_state();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(1ms);
}
