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
#include "mock_performance_booster.h"

#include <gmock/gmock.h>

namespace rt = repowerd::test;

namespace
{

struct APerformanceBooster : rt::AcceptanceTest
{
    void expect_enable_interactive_mode()
    {
        EXPECT_CALL(*config.the_mock_performance_booster(), enable_interactive_mode());
    }

    void expect_disable_interactive_mode()
    {
        EXPECT_CALL(*config.the_mock_performance_booster(), disable_interactive_mode());
    }
};

}

TEST_F(APerformanceBooster, interactive_mode_is_enabled_when_display_turns_on)
{
    expect_enable_interactive_mode();

    turn_on_display();
}

TEST_F(APerformanceBooster, interactive_mode_is_disabled_when_display_turns_off)
{
    turn_on_display();

    expect_disable_interactive_mode();
    turn_off_display();
}
