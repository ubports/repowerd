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

#include "src/adapters/real_chrono.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <functional>

using namespace testing;
using namespace std::chrono_literals;

namespace
{

struct ARealChrono : Test
{
    repowerd::RealChrono real_chrono;

    std::chrono::milliseconds duration_of(std::function<void()> const& func)
    {
        auto start = std::chrono::steady_clock::now();
        func();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start);
    }
};

MATCHER_P(IsAbout, a, "")
{
    return arg >= a && arg <= a + 10ms;
}

}

TEST_F(ARealChrono, sleeps_for_right_amount_of_time)
{
    EXPECT_THAT(duration_of([this]{real_chrono.sleep_for(50ms);}), IsAbout(50ms));
}
