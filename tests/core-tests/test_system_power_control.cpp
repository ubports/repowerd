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
#include "fake_system_power_control.h"

#include <gtest/gtest.h>

namespace rt = repowerd::test;

using namespace std::chrono_literals;
using namespace testing;

namespace
{

struct ASystemPowerControl : rt::AcceptanceTest
{
    void expect_automatic_suspend_is_allowed()
    {
        EXPECT_TRUE(config.the_fake_system_power_control()->is_automatic_suspend_allowed());
    }

    void expect_automatic_suspend_is_disallowed()
    {
        EXPECT_FALSE(config.the_fake_system_power_control()->is_automatic_suspend_allowed());
    }

    void expect_automatic_suspend_disallow()
    {
        EXPECT_CALL(config.the_fake_system_power_control()->mock,
                    disallow_automatic_suspend(_));
    }

    void emit_system_resume()
    {
        config.the_fake_system_power_control()->emit_system_resume();
        daemon.flush();
    }

    std::chrono::milliseconds const suspend_timeout{
        user_inactivity_normal_suspend_timeout + 10s};
};

}

TEST_F(ASystemPowerControl, automatic_suspend_is_disallowed_when_display_turns_on)
{
    expect_automatic_suspend_is_allowed();

    turn_on_display();

    expect_automatic_suspend_is_disallowed();
}

TEST_F(ASystemPowerControl, automatic_suspend_is_allowed_when_display_turns_off)
{
    turn_on_display();
    turn_off_display();

    expect_automatic_suspend_is_allowed();
}

TEST_F(ASystemPowerControl,
       automatic_suspend_is_disallowed_when_display_turns_off_due_to_proximity)
{
    turn_on_display();
    emit_proximity_state_near();

    expect_automatic_suspend_is_disallowed();
}

TEST_F(ASystemPowerControl,
       suspend_inhibition_is_respected_for_automatic_suspend_due_to_inactivity)
{
    turn_on_display();
    client_request_disallow_suspend();
    advance_time_by(user_inactivity_normal_display_off_timeout);

    expect_automatic_suspend_is_disallowed();
}

TEST_F(ASystemPowerControl,
       suspend_inhibition_is_respected_for_automatic_suspend_due_to_power_key)
{
    turn_on_display();
    client_request_disallow_suspend();

    press_power_button();
    release_power_button();

    expect_automatic_suspend_is_disallowed();
}

TEST_F(ASystemPowerControl,
       removal_of_suspend_inhibition_is_respected_for_automatic_suspend_due_to_inactivity)
{
    turn_on_display();
    client_request_disallow_suspend();

    press_power_button();
    release_power_button();

    client_request_allow_suspend();
    expect_automatic_suspend_is_allowed();
}

TEST_F(ASystemPowerControl,
       removal_of_suspend_inhibition_is_respected_for_automatic_suspend_due_to_power_key)
{
    turn_on_display();
    client_request_disallow_suspend();

    advance_time_by(user_inactivity_normal_display_off_timeout);

    client_request_allow_suspend();
    expect_automatic_suspend_is_allowed();
}

TEST_F(ASystemPowerControl,
       removal_of_suspend_inhibition_has_no_effect_for_automatic_suspend_due_to_proximity)
{
    turn_on_display();
    client_request_disallow_suspend();

    emit_proximity_state_near();

    client_request_allow_suspend();

    expect_automatic_suspend_is_disallowed();
}

TEST_F(ASystemPowerControl, automatic_suspend_is_disallowed_before_display_is_turned_on)
{
    testing::InSequence s;
    expect_automatic_suspend_disallow();
    expect_display_turns_on();

    press_power_button();
    release_power_button();
}

TEST_F(ASystemPowerControl, suspend_inhibit_disallow_automatic_suspend_by_itself)
{
    testing::InSequence s;
    expect_automatic_suspend_is_allowed();

    client_request_disallow_suspend();
    expect_automatic_suspend_is_disallowed();
}

TEST_F(ASystemPowerControl, system_disallow_suspend_inhibits_suspend_due_to_inactivity)
{
    turn_on_display();

    client_setting_set_inactivity_behavior(
        repowerd::PowerAction::suspend,
        repowerd::PowerSupply::battery,
        suspend_timeout);

    emit_system_disallow_suspend();

    expect_no_system_power_change();
    advance_time_by(suspend_timeout);

    expect_system_suspends();
    emit_system_allow_suspend();
}

TEST_F(ASystemPowerControl, resume_turns_on_screen)
{
    expect_display_turns_on();

    emit_system_resume();
}

TEST_F(ASystemPowerControl, resume_is_logged)
{
    emit_system_resume();

    EXPECT_TRUE(log_contains_line({"system_resume"}));
}
