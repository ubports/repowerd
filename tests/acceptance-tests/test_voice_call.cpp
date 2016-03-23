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

using namespace std::chrono_literals;

namespace
{

struct AVoiceCall : rt::AcceptanceTest
{
    void emit_proximity_state_far_if_enabled()
    {
        config.the_fake_proximity_sensor()->emit_proximity_state_if_enabled(
            repowerd::ProximityState::far);
    }

    void emit_proximity_state_near_if_enabled()
    {
        config.the_fake_proximity_sensor()->emit_proximity_state_if_enabled(
            repowerd::ProximityState::near);
    }
};

}

TEST_F(AVoiceCall, turns_on_display_with_normal_timeout)
{
    expect_display_turns_on();
    emit_active_call();
    verify_expectations();

    expect_no_display_power_change();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(1ms);
}

TEST_F(AVoiceCall, brightens_display_if_it_is_already_on)
{
    turn_on_display();

    expect_display_dims();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);
    verify_expectations();

    expect_no_display_power_change();
    expect_display_brightens();
    emit_active_call();
}

TEST_F(AVoiceCall, extends_timeout_if_display_is_already_on)
{
    turn_on_display();

    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);

    expect_no_display_power_change();
    emit_active_call();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(1ms);
}

TEST_F(AVoiceCall, enables_proximity_events)
{
    emit_active_call();

    expect_display_turns_off();
    emit_proximity_state_near_if_enabled();
    verify_expectations();

    expect_display_turns_on();
    emit_proximity_state_far_if_enabled();
    verify_expectations();
}

TEST_F(AVoiceCall, when_done_turns_on_display_with_reduced_timeout)
{
    emit_active_call();
    advance_time_by(user_inactivity_normal_display_off_timeout);

    expect_display_turns_on();
    emit_no_active_call();
    verify_expectations();

    expect_no_display_power_change();
    advance_time_by(user_inactivity_reduced_display_off_timeout - 1ms);
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(1ms);
}

TEST_F(AVoiceCall, when_done_brightens_display_if_it_is_already_on)
{
    emit_active_call();

    expect_display_dims();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);
    verify_expectations();

    expect_no_display_power_change();
    expect_display_brightens();
    emit_no_active_call();
}

TEST_F(AVoiceCall, when_done_extends_timeout_if_display_is_already_on)
{
    emit_active_call();

    expect_no_display_power_change();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);
    emit_no_active_call();
    verify_expectations();

    expect_no_display_power_change();
    advance_time_by(user_inactivity_reduced_display_off_timeout - 1ms);
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(1ms);
}

TEST_F(AVoiceCall, when_done_disables_proximity_events)
{
    emit_active_call();
    emit_no_active_call();

    expect_no_display_power_change();
    emit_proximity_state_near_if_enabled();
    emit_proximity_state_far_if_enabled();
    verify_expectations();
}
