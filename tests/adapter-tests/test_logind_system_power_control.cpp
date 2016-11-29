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

#include "dbus_bus.h"
#include "fake_log.h"
#include "fake_shared.h"
#include "fake_logind.h"

#include "src/adapters/logind_system_power_control.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>
#include <unordered_set>

namespace rt = repowerd::test;
using namespace testing;
using namespace std::chrono_literals;

namespace
{

struct ALogindSystemPowerControl : testing::Test
{
    rt::DBusBus bus;
    rt::FakeLog fake_log;
    rt::FakeLogind fake_logind{bus.address()};
    repowerd::LogindSystemPowerControl system_power_control{
        rt::fake_shared(fake_log), bus.address()};

    void expect_inhibitions(std::unordered_set<std::string> const& inhibitions)
    {
        EXPECT_THAT(fake_logind.active_inhibitions(), ContainerEq(inhibitions));
    }

    void expect_power_off_requests(std::string const& power_off_requests)
    {
        EXPECT_THAT(fake_logind.power_off_requests(), StrEq(power_off_requests));
    }

    std::string inhibition_name_for_id(std::string const& id)
    {
        return "sleep:idle,repowerd," + id + ",block";
    }

    std::chrono::seconds const default_timeout{3};
};

}

TEST_F(ALogindSystemPowerControl, disallow_any_suspend_adds_inhibition)
{
    system_power_control.disallow_suspend("id1", repowerd::SuspendType::any);
    system_power_control.disallow_suspend("id2", repowerd::SuspendType::any);

    expect_inhibitions(
        {
            inhibition_name_for_id("id1"),
            inhibition_name_for_id("id2")
        });
}

TEST_F(ALogindSystemPowerControl, allow_any_suspend_removes_inhibition)
{
    system_power_control.disallow_suspend("id1", repowerd::SuspendType::any);
    system_power_control.disallow_suspend("id2", repowerd::SuspendType::any);
    system_power_control.disallow_suspend("id3", repowerd::SuspendType::any);

    system_power_control.allow_suspend("id2", repowerd::SuspendType::any);
    expect_inhibitions(
        {
            inhibition_name_for_id("id1"),
            inhibition_name_for_id("id3")
        });

    system_power_control.allow_suspend("id1", repowerd::SuspendType::any);
    expect_inhibitions(
        {
            inhibition_name_for_id("id3")
        });

    system_power_control.allow_suspend("id3", repowerd::SuspendType::any);
    expect_inhibitions({});
}

TEST_F(ALogindSystemPowerControl, disallow_automatic_suspend_is_a_noop)
{
    system_power_control.disallow_suspend("id1", repowerd::SuspendType::automatic);

    expect_inhibitions({});
}

TEST_F(ALogindSystemPowerControl, allow_automatic_suspend_is_a_noop)
{
    system_power_control.disallow_suspend("id1", repowerd::SuspendType::any);
    system_power_control.allow_suspend("id1", repowerd::SuspendType::automatic);

    expect_inhibitions({inhibition_name_for_id("id1")});
}

TEST_F(ALogindSystemPowerControl, powers_off_system)
{
    system_power_control.power_off();

    expect_power_off_requests("f");
}

TEST_F(ALogindSystemPowerControl, disallow_any_suspend_logs_inhibition)
{
    system_power_control.disallow_suspend("id1", repowerd::SuspendType::any);

    EXPECT_TRUE(fake_log.contains_line({"inhibit", "sleep:idle", "id1"}));
    EXPECT_TRUE(fake_log.contains_line({"inhibit", "sleep:idle", "id1", "done"}));
}

TEST_F(ALogindSystemPowerControl, allow_any_suspend_logs_inhibition_release)
{
    system_power_control.allow_suspend("id1", repowerd::SuspendType::any);

    EXPECT_TRUE(fake_log.contains_line({"releasing", "inhibition", "id1"}));
}

TEST_F(ALogindSystemPowerControl, allow_and_disallow_automatic_suspend_log_nothing)
{
    system_power_control.allow_suspend("id1", repowerd::SuspendType::automatic);
    system_power_control.disallow_suspend("id1", repowerd::SuspendType::automatic);

    EXPECT_FALSE(fake_log.contains_line({"inhibit"}));
}

TEST_F(ALogindSystemPowerControl, power_off_is_logged)
{
    system_power_control.power_off();

    EXPECT_TRUE(fake_log.contains_line({"power_off"}));
    EXPECT_TRUE(fake_log.contains_line({"power_off", "done"}));
}
