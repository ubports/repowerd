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
    void expect_suspend_when_allowed()
    {
        EXPECT_CALL(config.the_fake_system_power_control()->mock,
                    suspend_when_allowed(_));
    }

    void expect_cancel_suspend_when_allowed()
    {
        EXPECT_CALL(config.the_fake_system_power_control()->mock,
                    cancel_suspend_when_allowed(_));
    }

    void expect_no_suspend_when_allowed()
    {
        EXPECT_CALL(config.the_fake_system_power_control()->mock,
                    suspend_when_allowed(_)).Times(0);
    }

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
    expect_suspend_when_allowed();

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
    expect_cancel_suspend_when_allowed();
    open_lid();
}

TEST_F(ALid, opened_is_logged)
{
    open_lid();

    EXPECT_TRUE(log_contains_line({"lid_open"}));
}

TEST_F(ALid, closed_does_not_turns_off_display_nor_suspend_if_external_displays_active)
{
    activate_external_display();
    turn_on_display();

    expect_no_display_power_change();
    expect_no_suspend_when_allowed();

    close_lid();
}
