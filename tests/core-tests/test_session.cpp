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
#include "default_pid.h"

#include <gtest/gtest.h>

namespace rt = repowerd::test;
using namespace std::chrono_literals;

namespace
{

struct ASession : rt::AcceptanceTest
{
    ASession()
    {
        add_incompatible_session(incompatible(0), incompatible_pid(0));
        add_compatible_session(compatible(0), compatible_pid(0));
        add_compatible_session(compatible(1), compatible_pid(1));
    }

    std::string compatible(int i)
    {
        return "compatible" + std::to_string(i);
    }

    pid_t compatible_pid(int i)
    {
        return rt::default_pid + 100 + i;
    }

    std::string incompatible(int i)
    {
        return "incompatible" + std::to_string(i);
    }

    pid_t incompatible_pid(int i)
    {
        return rt::default_pid + 200 + i;
    }
};

}

TEST_F(ASession, switch_to_incompatible_session_disables_inactivity_timeouts)
{
    turn_on_display();
    switch_to_session(incompatible(0));

    expect_no_display_power_change();
    expect_no_display_brightness_change();
    advance_time_by(user_inactivity_normal_display_off_timeout);
}

TEST_F(ASession, switch_to_known_compatible_session_turns_on_display_with_normal_timeout)
{
    expect_no_display_power_change();
    switch_to_session(incompatible(0));
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);
    verify_expectations();

    expect_display_turns_on();
    switch_to_session(default_session_id);
    verify_expectations();

    expect_no_display_power_change();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);
}

TEST_F(ASession, switch_to_session_without_active_call_disables_proximity)
{
    emit_active_call();
    switch_to_session(compatible(0));

    expect_no_display_power_change();
    expect_no_display_brightness_change();
    emit_proximity_state_near_if_enabled();
    emit_proximity_state_far_if_enabled();
}

TEST_F(ASession, switch_back_to_session_with_active_call_reenables_proximity)
{
    auto const without_call = 0;

    emit_active_call();

    switch_to_session(compatible(without_call));
    switch_to_session(default_session_id);

    expect_display_turns_off();
    emit_proximity_state_near_if_enabled();
}

TEST_F(ASession,
       switch_back_to_sessions_with_call_finished_while_inactive_does_not_reenable_proximity)
{
    auto const with_initially_active_call = 0;
    auto const another_with_initially_active_call = 1;

    switch_to_session(compatible(with_initially_active_call));
    emit_active_call();

    switch_to_session(compatible(another_with_initially_active_call));
    emit_active_call();

    switch_to_session(default_session_id);
    emit_no_active_call();

    switch_to_session(compatible(with_initially_active_call));
    expect_no_display_power_change();
    expect_no_display_brightness_change();
    emit_proximity_state_near_if_enabled();
    emit_proximity_state_far_if_enabled();
    verify_expectations();

    switch_to_session(compatible(another_with_initially_active_call));
    expect_no_display_power_change();
    expect_no_display_brightness_change();
    emit_proximity_state_near_if_enabled();
    emit_proximity_state_far_if_enabled();
    verify_expectations();
}

TEST_F(ASession, switch_to_incompatible_session_disables_autobrightness)
{
    expect_autobrightness_disabled();

    switch_to_session(incompatible(0));
}

TEST_F(ASession,
       while_inactive_does_not_change_display_for_inactivity_timeout_requests_if_display_is_off)
{
    switch_to_session(incompatible(0));

    expect_no_display_power_change();
    expect_no_display_brightness_change();

    client_request_disable_inactivity_timeout();
    client_request_enable_inactivity_timeout();
    client_request_set_inactivity_timeout(10s);

    advance_time_by(1000s);
}

TEST_F(ASession,
       while_inactive_does_not_change_display_for_inactivity_timeout_requests_if_display_is_on)
{
    turn_on_display();

    switch_to_session(incompatible(0));

    expect_no_display_power_change();
    expect_no_display_brightness_change();

    client_request_disable_inactivity_timeout();
    client_request_enable_inactivity_timeout();
    client_request_set_inactivity_timeout(10s);

    advance_time_by(1000s);
}

TEST_F(ASession, while_inactive_tracks_inactivity_timeout_requests)
{
    switch_to_session(incompatible(0));

    auto const timeout = 1000s;
    client_request_disable_inactivity_timeout("id");
    client_request_set_inactivity_timeout(timeout);

    switch_to_session(default_session_id);

    expect_no_display_power_change();
    expect_no_display_brightness_change();
    advance_time_by(timeout);
    client_request_enable_inactivity_timeout("id");
    verify_expectations();

    expect_display_dims();
    advance_time_by(timeout - 1ms);
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(1ms);
}

TEST_F(ASession, while_inactive_tracks_inactivity_timeout_requests_1)
{
    switch_to_session(incompatible(0));

    auto const timeout = 1000s;
    client_request_disable_inactivity_timeout("id");
    client_request_set_inactivity_timeout(timeout);
    client_request_enable_inactivity_timeout("id");

    switch_to_session(default_session_id);

    expect_display_turns_off();
    advance_time_by(timeout);
}

TEST_F(ASession,
       while_inactive_does_not_change_display_for_brightness_requests_if_display_was_off)
{
    switch_to_session(incompatible(0));

    expect_no_display_power_change();
    expect_no_display_brightness_change();
    expect_no_autobrightness_change();

    client_request_set_normal_brightness_value(0.66);
    client_request_enable_autobrightness();
    client_request_disable_autobrightness();
}

TEST_F(ASession,
       while_inactive_does_not_change_display_for_brightness_requests_if_display_was_on)
{
    turn_on_display();
    switch_to_session(incompatible(0));

    expect_no_display_power_change();
    expect_no_display_brightness_change();
    expect_no_autobrightness_change();

    client_request_set_normal_brightness_value(0.66);
    client_request_enable_autobrightness();
    client_request_disable_autobrightness();
}

TEST_F(ASession, while_inactive_tracks_brightness_requests)
{
    switch_to_session(incompatible(0));

    client_request_set_normal_brightness_value(0.66);
    client_request_enable_autobrightness();
    client_request_disable_autobrightness();

    expect_autobrightness_disabled();
    expect_normal_brightness_value_set_to(0.66);
    switch_to_session(default_session_id);
}

TEST_F(ASession,
       while_inactive_does_not_change_display_for_notification_if_display_is_on)
{
    turn_on_display();

    switch_to_session(incompatible(0));

    expect_no_display_power_change();
    expect_no_display_brightness_change();

    emit_notification();
    emit_notification_done();
}

TEST_F(ASession,
       while_inactive_does_not_change_display_for_notification_if_display_is_off)
{
    switch_to_session(incompatible(0));

    expect_no_display_power_change();
    expect_no_display_brightness_change();

    emit_notification();
    emit_notification_done();
}

TEST_F(ASession,
       while_inactive_does_not_change_display_for_notification_expiration)
{
    emit_notification();

    switch_to_session(incompatible(0));

    expect_no_display_power_change();
    expect_no_display_brightness_change();
    advance_time_by(notification_expiration_timeout);
}

TEST_F(ASession, while_inactive_tracks_notification_expiration)
{
    emit_notification();

    switch_to_session(incompatible(0));

    advance_time_by(notification_expiration_timeout);

    switch_to_session(default_session_id);

    expect_display_turns_off();
    advance_time_by(user_inactivity_normal_display_off_timeout);
}

TEST_F(ASession, while_inactive_does_not_track_power_button_long_press)
{
    press_power_button();

    switch_to_session(incompatible(0));
    switch_to_session(default_session_id);

    expect_no_long_press_notification();
    advance_time_by(power_button_long_press_timeout);
}
