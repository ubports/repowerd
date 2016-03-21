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
#include "fake_notification_service.h"

#include <gtest/gtest.h>

#include <chrono>

namespace rt = repowerd::test;

using namespace std::chrono_literals;

namespace
{

struct ANotification : rt::AcceptanceTest
{
    void emit_notification()
    {
        config.the_fake_notification_service()->emit_notification();
    }

    std::chrono::milliseconds const user_inactivity_normal_display_off_timeout{
        config.user_inactivity_normal_display_off_timeout()};
    std::chrono::milliseconds const user_inactivity_reduced_display_off_timeout{
        config.user_inactivity_reduced_display_off_timeout()};
};

}

TEST_F(ANotification, turns_on_display_with_reduced_timeout)
{
    expect_display_turns_on();
    emit_notification();
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(user_inactivity_reduced_display_off_timeout);
}

TEST_F(ANotification, does_not_turn_on_display_if_display_is_already_on)
{
    turn_on_display();

    expect_no_display_power_change();
    emit_notification();
}

TEST_F(ANotification, does_not_dim_display_after_timeout)
{
    expect_display_turns_on();
    emit_notification();
    verify_expectations();

    expect_no_display_brightness_change();
    advance_time_by(user_inactivity_reduced_display_off_timeout - 1ms);
}

TEST_F(ANotification, extends_existing_shorter_timeout)
{
    turn_on_display();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);

    emit_notification();

    expect_no_display_power_change();
    advance_time_by(1ms);
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(user_inactivity_reduced_display_off_timeout);
}

TEST_F(ANotification, does_not_reduce_existing_longer_timeout)
{
    turn_on_display();
    advance_time_by(
        user_inactivity_normal_display_off_timeout -
        user_inactivity_reduced_display_off_timeout -
        1ms);

    emit_notification();

    expect_no_display_power_change();
    advance_time_by(user_inactivity_reduced_display_off_timeout);
    verify_expectations();
}

TEST_F(ANotification, does_not_turn_on_display_when_proximity_is_near)
{
    set_proximity_state_near();

    expect_no_display_power_change();
    emit_notification();
}

TEST_F(ANotification, does_not_schedule_inactivity_timeout_when_proximity_is_near)
{
    set_proximity_state_near();

    expect_no_display_power_change();
    emit_notification();
    advance_time_by(user_inactivity_reduced_display_off_timeout);
}

TEST_F(ANotification, reduced_timeout_is_extended_by_user_activity)
{
    expect_display_turns_on();
    emit_notification();
    verify_expectations();

    expect_no_display_power_change();
    perform_user_activity_extending_power_state();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(1ms);
}

TEST_F(ANotification, allows_power_button_to_turn_off_display)
{
    expect_display_turns_on();
    emit_notification();
    verify_expectations();

    expect_display_turns_off();
    press_power_button();
    release_power_button();
    verify_expectations();
}

TEST_F(ANotification, brightens_dim_display)
{
    turn_on_display();

    expect_display_dims();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);
    verify_expectations();

    expect_display_brightens();
    emit_notification();
}
