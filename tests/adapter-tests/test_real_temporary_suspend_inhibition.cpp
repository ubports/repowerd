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

#include "src/adapters/real_temporary_suspend_inhibition.h"

#include "fake_shared.h"
#include "fake_suspend_control.h"
#include "spin_wait.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <functional>

namespace rt = repowerd::test;

using namespace testing;
using namespace std::chrono_literals;

namespace
{

struct ARealTemporarySuspendInhibition : Test
{
    rt::FakeSuspendControl fake_suspend_control;
    repowerd::RealTemporarySuspendInhibition real_temporary_suspend_inhbition{
        rt::fake_shared(fake_suspend_control)};

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

TEST_F(ARealTemporarySuspendInhibition, disallows_suspend)
{
    real_temporary_suspend_inhbition.inhibit_suspend_for(100s, "bla");
    EXPECT_FALSE(fake_suspend_control.is_suspend_allowed());
}

TEST_F(ARealTemporarySuspendInhibition, reallows_suspend_after_timeout)
{
    EXPECT_THAT(duration_of(
        [this]
        {
            real_temporary_suspend_inhbition.inhibit_suspend_for(50ms, "bla");
            rt::spin_wait_for_condition_or_timeout(
                [this] { return fake_suspend_control.is_suspend_allowed(); },
                3s);
        }),
        IsAbout(50ms));
}

TEST_F(ARealTemporarySuspendInhibition, handles_concurrent_suspend_inhibitions)
{
    EXPECT_THAT(duration_of(
        [this]
        {
            real_temporary_suspend_inhbition.inhibit_suspend_for(100ms, "bla");
            real_temporary_suspend_inhbition.inhibit_suspend_for(100ms, "bla1");
            real_temporary_suspend_inhbition.inhibit_suspend_for(50ms, "bla");
            real_temporary_suspend_inhbition.inhibit_suspend_for(50ms, "bla1");

            rt::spin_wait_for_condition_or_timeout(
                [this] { return fake_suspend_control.is_suspend_allowed(); },
                3s);
        }),
        IsAbout(100ms));
}
