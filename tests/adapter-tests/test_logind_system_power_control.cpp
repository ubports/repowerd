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
#include "spin_wait.h"

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

    ALogindSystemPowerControl()
    {
        system_power_control.start_processing();
    }

    void expect_inhibitions(std::unordered_set<std::string> const& inhibitions)
    {
        rt::spin_wait_for_condition_or_timeout(
            [&] { return fake_logind.active_inhibitions() == inhibitions; },
            default_timeout);
        EXPECT_THAT(fake_logind.active_inhibitions(), ContainerEq(inhibitions));
    }

    void expect_no_inhibitions()
    {
        rt::spin_wait_for_condition_or_timeout(
            [&] { return fake_logind.active_inhibitions().empty(); },
            default_timeout);
        EXPECT_THAT(fake_logind.active_inhibitions(), IsEmpty());
    }

    void expect_power_requests(std::string const& power_requests)
    {
        EXPECT_THAT(fake_logind.power_requests(), StrEq(power_requests));
    }

    std::string inhibition_name_for_id(std::string const& id)
    {
        return "sleep:idle,repowerd," + id + ",block";
    }

    std::string idle_and_lid_inhibition_name()
    {
        return "idle:handle-lid-switch,repowerd,repowerd handles idle and lid,block";
    }

    std::chrono::seconds const default_timeout{3};
};

}

TEST_F(ALogindSystemPowerControl, disallow_any_suspend_does_not_add_inhibition)
{
    system_power_control.disallow_suspend("id1", repowerd::SuspendType::any);
    system_power_control.disallow_suspend("id2", repowerd::SuspendType::any);

    expect_no_inhibitions();
}

TEST_F(ALogindSystemPowerControl, disallow_automatic_suspend_is_a_noop)
{
    system_power_control.disallow_suspend("id1", repowerd::SuspendType::automatic);

    expect_no_inhibitions();
}

TEST_F(ALogindSystemPowerControl, allow_automatic_suspend_is_a_noop)
{
    system_power_control.allow_suspend("id1", repowerd::SuspendType::automatic);

    expect_no_inhibitions();
}

TEST_F(ALogindSystemPowerControl, powers_off_system)
{
    system_power_control.power_off();

    expect_power_requests("[power-off:f]");
}

TEST_F(ALogindSystemPowerControl, suspends_system_regardless_of_disallowances)
{
    system_power_control.disallow_suspend("id1", repowerd::SuspendType::any);

    expect_power_requests("");

    system_power_control.suspend();

    expect_power_requests("[suspend:f]");
}

TEST_F(ALogindSystemPowerControl, suspend_if_allowed_suspends_if_no_disallowances)
{
    expect_power_requests("");

    system_power_control.suspend_if_allowed();

    expect_power_requests("[suspend:f]");
}

TEST_F(ALogindSystemPowerControl, suspend_if_allowed_does_not_suspend_if_there_are_disallowances)
{
    expect_power_requests("");

    system_power_control.disallow_suspend("id1", repowerd::SuspendType::any);
    system_power_control.suspend_if_allowed();

    expect_power_requests("");
}

TEST_F(ALogindSystemPowerControl,
       suspend_if_allowed_suspends_after_disallowance_have_been_removed)
{
    expect_power_requests("");

    system_power_control.disallow_suspend("id1", repowerd::SuspendType::any);
    system_power_control.disallow_suspend("id2", repowerd::SuspendType::any);
    system_power_control.allow_suspend("id1", repowerd::SuspendType::any);
    system_power_control.allow_suspend("id2", repowerd::SuspendType::any);

    system_power_control.suspend_if_allowed();

    expect_power_requests("[suspend:f]");
}

TEST_F(ALogindSystemPowerControl,
       suspend_when_allowed_suspends_immediately_if_allowed)
{
    expect_power_requests("");

    system_power_control.suspend_when_allowed("when_allowed");

    expect_power_requests("[suspend:f]");
}

TEST_F(ALogindSystemPowerControl,
       suspend_when_allowed_suspends_when_allowed_later)
{
    system_power_control.disallow_suspend("id1", repowerd::SuspendType::any);
    system_power_control.suspend_when_allowed("when_allowed");

    expect_power_requests("");

    system_power_control.allow_suspend("id1", repowerd::SuspendType::any);

    expect_power_requests("[suspend:f]");
}

TEST_F(ALogindSystemPowerControl,
       cancel_suspend_when_allowed_cancels_pending_suspend)
{
    system_power_control.disallow_suspend("id1", repowerd::SuspendType::any);
    system_power_control.suspend_when_allowed("when_allowed");
    system_power_control.cancel_suspend_when_allowed("when_allowed");
    system_power_control.allow_suspend("id1", repowerd::SuspendType::any);

    expect_power_requests("");
}

TEST_F(ALogindSystemPowerControl, disallow_default_system_handlers_adds_inhibition)
{
    expect_no_inhibitions();

    system_power_control.disallow_default_system_handlers();

    expect_inhibitions({idle_and_lid_inhibition_name()});
}

TEST_F(ALogindSystemPowerControl, allow_default_system_handlers_removes_inhibition)
{
    system_power_control.disallow_default_system_handlers();
    system_power_control.allow_default_system_handlers();

    expect_no_inhibitions();
}

TEST_F(ALogindSystemPowerControl, notifies_of_system_resume)
{
    std::atomic<bool> system_resume{false};

    auto const system_resume_registration =
        system_power_control.register_system_resume_handler(
            [&] { system_resume = true; });

    fake_logind.emit_prepare_for_sleep(false);

    rt::spin_wait_for_condition_or_timeout(
        [&] { return system_resume.load(); },
        default_timeout);

    EXPECT_THAT(system_resume, Eq(true));
    EXPECT_TRUE(fake_log.contains_line({"PrepareForSleep", "false"}));
}

TEST_F(ALogindSystemPowerControl, disallow_any_suspend_is_logged)
{
    system_power_control.disallow_suspend("id1", repowerd::SuspendType::any);

    EXPECT_TRUE(fake_log.contains_line({"disallow_suspend", "id1", "any"}));
}

TEST_F(ALogindSystemPowerControl, allow_any_suspend_is_logged)
{
    system_power_control.allow_suspend("id1", repowerd::SuspendType::any);

    EXPECT_TRUE(fake_log.contains_line({"allow_suspend", "id1", "any"}));
}

TEST_F(ALogindSystemPowerControl, allow_and_disallow_automatic_suspend_log_nothing)
{
    system_power_control.allow_suspend("id1", repowerd::SuspendType::automatic);
    system_power_control.disallow_suspend("id1", repowerd::SuspendType::automatic);

    EXPECT_FALSE(fake_log.contains_line({"id1"}));
}

TEST_F(ALogindSystemPowerControl, disallow_default_system_handlers_logs_inhibition)
{
    system_power_control.disallow_default_system_handlers();

    EXPECT_TRUE(fake_log.contains_line({"inhibit", "idle:handle-lid-switch"}));
    EXPECT_TRUE(fake_log.contains_line({"inhibit", "idle:handle-lid-switch", "done"}));
}

TEST_F(ALogindSystemPowerControl, allow_default_system_handlers_logs_inhibition_release)
{
    system_power_control.allow_default_system_handlers();

    EXPECT_TRUE(fake_log.contains_line({"releasing", "idle", "lid", "inhibition"}));
}

TEST_F(ALogindSystemPowerControl, power_off_is_logged)
{
    system_power_control.power_off();

    EXPECT_TRUE(fake_log.contains_line({"power_off"}));
    EXPECT_TRUE(fake_log.contains_line({"power_off", "done"}));
}

TEST_F(ALogindSystemPowerControl, suspend_is_logged)
{
    system_power_control.suspend_if_allowed();

    EXPECT_TRUE(fake_log.contains_line({"suspend"}));
    EXPECT_TRUE(fake_log.contains_line({"suspend", "done"}));
}
