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

struct APowerButton : rt::AcceptanceTest
{
};

}

TEST_F(APowerButton, press_turns_on_display)
{
    expect_display_turns_on();

    press_power_button();
}

TEST_F(APowerButton, release_after_press_that_turns_on_display_does_nothing)
{
    expect_display_turns_on();

    press_power_button();
    release_power_button();
}

TEST_F(APowerButton, stray_release_is_ignored_when_display_is_off)
{
    expect_no_display_power_change();

    release_power_button();
}

TEST_F(APowerButton, stray_release_is_ignored_when_display_is_on)
{
    turn_on_display();

    expect_no_display_power_change();

    release_power_button();
}

TEST_F(APowerButton, press_does_nothing_if_display_is_already_on)
{
    turn_on_display();

    expect_no_display_power_change();

    press_power_button();
}

TEST_F(APowerButton, short_press_turns_off_display_it_is_already_on)
{
    turn_on_display();

    expect_display_turns_off();

    press_power_button();
    release_power_button();
}

TEST_F(APowerButton, long_press_turns_on_display_and_notifies_if_display_is_off)
{
    expect_display_turns_on();
    expect_long_press_notification();

    press_power_button();
    advance_time_by(power_button_long_press_timeout);
}

TEST_F(APowerButton, long_press_notifies_if_display_is_on)
{
    turn_on_display();

    expect_no_display_power_change();
    expect_long_press_notification();

    press_power_button();
    advance_time_by(power_button_long_press_timeout);
}

TEST_F(APowerButton, short_press_can_change_display_power_state_after_long_press)
{
    turn_on_display();

    expect_long_press_notification();
    press_power_button();
    advance_time_by(power_button_long_press_timeout);
    release_power_button();
    verify_expectations();

    expect_display_turns_off();
    press_power_button();
    release_power_button();
    verify_expectations();

    expect_display_turns_on();
    press_power_button();
    release_power_button();
    verify_expectations();
}

TEST_F(APowerButton, event_notifies_of_display_power_change)
{
    expect_display_power_on_notification(
        repowerd::DisplayPowerChangeReason::power_button);

    press_power_button();
    release_power_button();
    verify_expectations();

    expect_display_power_off_notification(
        repowerd::DisplayPowerChangeReason::power_button);

    press_power_button();
    release_power_button();
}

TEST_F(APowerButton,
       press_turns_on_display_and_keeps_it_on_forever_if_inactivity_timeout_is_infinite)
{
    client_request_set_inactivity_timeout(infinite_timeout);

    turn_on_display();

    expect_no_display_power_change();
    expect_no_display_brightness_change();
    advance_time_by(1h);
}

TEST_F(APowerButton, press_is_logged)
{
    press_power_button();

    EXPECT_TRUE(log_contains_line({"power_button_press"}));
}

TEST_F(APowerButton, release_is_logged)
{
    release_power_button();

    EXPECT_TRUE(log_contains_line({"power_button_release"}));
}

TEST_F(APowerButton, long_press_is_logged)
{
    press_power_button();
    advance_time_by(power_button_long_press_timeout);

    EXPECT_TRUE(log_contains_line({"long_press"}));
}
