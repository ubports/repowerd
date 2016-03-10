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

#include "fake_proximity_sensor.h"

#include <gtest/gtest.h>

#include <chrono>

namespace rt = repowerd::test;

namespace
{

struct APromixitySensor : rt::AcceptanceTest
{
};

}

TEST_F(APromixitySensor, near_state_does_not_allow_display_to_turn_on_due_to_user_activity)
{
    expect_no_display_power_change();

    set_proximity_state_near();
    perform_user_activity_changing_power_state();
}

TEST_F(APromixitySensor, near_state_allows_display_to_turn_on_due_to_power_button)
{
    expect_display_turns_on();

    set_proximity_state_near();
    press_power_button();
    release_power_button();
}

TEST_F(APromixitySensor, far_event_turns_on_display)
{
    expect_display_turns_on();
    emit_proximity_state_far();
}

TEST_F(APromixitySensor, far_event_has_no_effect_if_display_is_on)
{
    turn_on_display();

    expect_no_display_power_change();

    emit_proximity_state_far();
}

TEST_F(APromixitySensor, near_event_turns_off_display)
{
    turn_on_display();

    expect_display_turns_off();

    emit_proximity_state_near();
}

TEST_F(APromixitySensor, near_event_has_no_effect_if_display_is_off)
{
    expect_no_display_power_change();

    emit_proximity_state_near();
}
