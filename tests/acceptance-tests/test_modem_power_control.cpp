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
#include "mock_modem_power_control.h"

#include <gtest/gtest.h>

namespace rt = repowerd::test;

namespace
{

struct AModemPowerControl : rt::AcceptanceTest
{
    void expect_modem_set_to_low_power_mode()
    {
        EXPECT_CALL(*config.the_mock_modem_power_control(), set_low_power_mode());
    }

    void expect_modem_set_to_normal_power_mode()
    {
        EXPECT_CALL(*config.the_mock_modem_power_control(), set_normal_power_mode());
    }

    void expect_no_modem_power_mode_change()
    {
        EXPECT_CALL(*config.the_mock_modem_power_control(), set_low_power_mode()).Times(0);
        EXPECT_CALL(*config.the_mock_modem_power_control(), set_normal_power_mode()).Times(0);
    }
};

}

TEST_F(AModemPowerControl, is_set_to_normal_power_mode_when_display_turns_on)
{
    expect_modem_set_to_normal_power_mode();

    turn_on_display();
}

TEST_F(AModemPowerControl, is_set_to_low_power_mode_when_display_turns_off)
{
    turn_on_display();

    expect_modem_set_to_low_power_mode();
    turn_off_display();
}

TEST_F(AModemPowerControl, is_left_to_normal_power_mode_when_display_turns_off_due_to_proximity)
{
    turn_on_display();

    expect_no_modem_power_mode_change();
    emit_proximity_state_near();
}
