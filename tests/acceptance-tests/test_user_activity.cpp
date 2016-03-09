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

#include <chrono>

namespace rt = repowerd::test;

namespace
{

struct AUserActivity : rt::AcceptanceTest
{
    std::chrono::milliseconds const user_inactivity_display_off_timeout{
        config.user_inactivity_display_off_timeout()};
};

}

TEST_F(AUserActivity, not_performed_allows_display_to_turn_off)
{
    turn_on_display();

    expect_display_turns_off();
    advance_time_by(user_inactivity_display_off_timeout);
}

TEST_F(AUserActivity, not_performed_has_no_effect_after_display_is_turned_off)
{
    turn_on_display();
    turn_off_display();

    expect_no_display_power_change();
    advance_time_by(user_inactivity_display_off_timeout);
}
