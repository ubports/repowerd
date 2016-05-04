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

struct AUserActivity : rt::AcceptanceTest
{
};

}

TEST_F(AUserActivity, not_performed_turns_off_display_after_timeout)
{
    turn_on_display();

    expect_display_turns_off();
    advance_time_by(user_inactivity_normal_display_off_timeout);
}

TEST_F(AUserActivity, not_performed_dims_display_after_timeout_timeout)
{
    turn_on_display();

    expect_display_dims();
    advance_time_by(user_inactivity_normal_display_dim_timeout);
}

TEST_F(AUserActivity, not_performed_does_not_turn_off_display_prematurely)
{
    turn_on_display();

    expect_no_display_power_change();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);
}

TEST_F(AUserActivity, not_performed_does_not_dim_display_prematurely)
{
    turn_on_display();

    expect_no_display_brightness_change();
    advance_time_by(user_inactivity_normal_display_dim_timeout - 1ms);
}

TEST_F(AUserActivity, not_performed_has_no_effect_after_display_is_turned_off)
{
    turn_on_display();
    turn_off_display();

    expect_no_display_brightness_change();
    expect_no_display_power_change();
    advance_time_by(user_inactivity_normal_display_off_timeout);
}

TEST_F(AUserActivity, extending_power_state_has_no_effect_when_display_is_off)
{
    expect_no_display_brightness_change();
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

TEST_F(AUserActivity, extending_power_state_brightens_dim_display)
{
    turn_on_display();

    expect_display_dims();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);
    verify_expectations();

    expect_display_brightens();
    perform_user_activity_extending_power_state();
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

TEST_F(AUserActivity, changing_power_state_brightens_dim_display)
{
    turn_on_display();

    expect_display_dims();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);
    verify_expectations();

    expect_display_brightens();
    perform_user_activity_changing_power_state();
}

TEST_F(AUserActivity, event_notifies_of_display_power_change)
{
    expect_display_power_on_notification(
        repowerd::DisplayPowerChangeReason::activity);
    perform_user_activity_changing_power_state();
    verify_expectations();

    expect_display_power_off_notification(
        repowerd::DisplayPowerChangeReason::activity);
    advance_time_by(user_inactivity_normal_display_off_timeout);
    verify_expectations();
}

TEST_F(AUserActivity,
       changing_power_state_keeps_screen_on_forever_if_inactivity_timeout_is_infinite)
{
    client_request_set_inactivity_timeout(infinite_timeout);

    emit_notification();
    emit_no_notification();

    perform_user_activity_changing_power_state();

    expect_no_display_power_change();
    expect_no_display_brightness_change();
    advance_time_by(1h);
}

TEST_F(AUserActivity,
       extending_power_state_keeps_screen_on_forever_if_inactivity_timeout_is_infinite)
{
    client_request_set_inactivity_timeout(infinite_timeout);

    emit_notification();
    emit_no_notification();

    perform_user_activity_extending_power_state();

    expect_no_display_power_change();
    expect_no_display_brightness_change();
    advance_time_by(1h);
}
