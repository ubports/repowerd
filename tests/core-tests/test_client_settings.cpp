/*
 * Copyright © 2017 Canonical Ltd.
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

struct AClientSetting : rt::AcceptanceTest
{
};

}

TEST_F(AClientSetting, to_set_inactivity_behavior_affects_display_off_timeout_on_battery)
{
    auto const display_off_timeout = user_inactivity_normal_display_off_timeout + 10s;

    client_setting_set_inactivity_behavior(
        repowerd::PowerAction::display_off,
        repowerd::PowerSupply::battery,
        display_off_timeout);

    turn_on_display();

    expect_no_display_power_change();
    advance_time_by(display_off_timeout - 1ms);
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(1ms);
}
