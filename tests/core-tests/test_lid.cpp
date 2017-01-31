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
#include "fake_display_information.h"
#include "fake_system_power_control.h"

#include <gtest/gtest.h>

namespace rt = repowerd::test;
using namespace testing;

namespace
{

struct ALid : rt::AcceptanceTest
{
    void activate_external_display()
    {
        config.the_fake_display_information()->set_has_active_external_displays(true);
    }
};

}

TEST_F(ALid, closed_turns_off_display_beforing_suspending_when_allowed)
{
    turn_on_display();

    InSequence s;
    expect_display_turns_off();
    expect_system_suspends_when_allowed("lid");

    close_lid();
}

TEST_F(ALid, closed_is_logged)
{
    close_lid();

    EXPECT_TRUE(log_contains_line({"lid_closed"}));
}

TEST_F(ALid, opened_turns_on_display)
{
    expect_display_turns_on();
    open_lid();
}

TEST_F(ALid, opened_cancels_suspend_when_allowed)
{
    expect_system_cancel_suspend_when_allowed("inactivity");
    expect_system_cancel_suspend_when_allowed("lid");
    open_lid();
}

TEST_F(ALid, opened_is_logged)
{
    open_lid();

    EXPECT_TRUE(log_contains_line({"lid_open"}));
}

TEST_F(ALid, closed_turns_off_internal_display_but_does_not_suspend_if_external_displays_active)
{
    activate_external_display();
    turn_on_display();

    expect_internal_display_turns_off();
    expect_no_system_power_change();

    close_lid();
}

TEST_F(ALid, opened_turns_on_internal_display_if_not_suspended)
{
    activate_external_display();
    turn_on_display();

    close_lid();

    expect_display_brightens();
    expect_internal_display_turns_on();

    open_lid();
}

TEST_F(ALid, while_closed_prevents_internal_display_from_turning_on)
{
    activate_external_display();
    turn_on_display();

    close_lid();

    expect_display_turns_off();
    advance_time_by(user_inactivity_normal_display_off_timeout);
    verify_expectations();

    expect_external_display_turns_on();
    perform_user_activity_changing_power_state();
}
