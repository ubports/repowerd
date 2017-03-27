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
    std::unique_ptr<repowerd::LogindSystemPowerControl> system_power_control;

    ALogindSystemPowerControl()
    {
        use_logind_with_initial_block_inhibited("");
    }

    void use_logind_with_initial_block_inhibited(std::string const& blocks)
    {
        registrations.clear();
        suspend_block_notifications.clear();

        fake_logind.set_block_inhibited(blocks);

        system_power_control =
            std::make_unique<repowerd::LogindSystemPowerControl>(
                rt::fake_shared(fake_log), bus.address());

        registrations.push_back(
            system_power_control->register_system_allow_suspend_handler(
                [&](std::string const& id)
                {
                    std::lock_guard<std::mutex> lock{mutex};
                    suspend_block_notifications += "[allow]";
                    suspend_block_ids.push_back(id);
                }));

        registrations.push_back(
            system_power_control->register_system_disallow_suspend_handler(
                [&](std::string const& id)
                {
                    std::lock_guard<std::mutex> lock{mutex};
                    suspend_block_notifications += "[disallow]";
                    suspend_block_ids.push_back(id);
                }));

        system_power_control->start_processing();
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

    void expect_suspend_block_notifications(std::string const& notifications)
    {
        EXPECT_TRUE(
            rt::spin_wait_for_condition_or_timeout(
                [&]
                {
                    std::lock_guard<std::mutex> lock{mutex};
                    return suspend_block_notifications == notifications;
                },
                default_timeout));
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
    std::vector<repowerd::HandlerRegistration> registrations;

    std::mutex mutex;
    std::string suspend_block_notifications;
    std::vector<std::string> suspend_block_ids;
};

}

TEST_F(ALogindSystemPowerControl, disallow_automatic_suspend_is_a_noop)
{
    system_power_control->disallow_automatic_suspend("id1");

    expect_no_inhibitions();
}

TEST_F(ALogindSystemPowerControl, allow_automatic_suspend_is_a_noop)
{
    system_power_control->allow_automatic_suspend("id1");

    expect_no_inhibitions();
}

TEST_F(ALogindSystemPowerControl, powers_off_system)
{
    system_power_control->power_off();

    expect_power_requests("[power-off:f]");
}

TEST_F(ALogindSystemPowerControl, suspends_system_regardless_of_disallowances)
{
    system_power_control->disallow_automatic_suspend("id1");

    expect_power_requests("");

    system_power_control->suspend();

    expect_power_requests("[suspend:f]");
}

TEST_F(ALogindSystemPowerControl, disallow_default_system_handlers_adds_inhibition)
{
    expect_no_inhibitions();

    system_power_control->disallow_default_system_handlers();

    expect_inhibitions({idle_and_lid_inhibition_name()});
}

TEST_F(ALogindSystemPowerControl, allow_default_system_handlers_removes_inhibition)
{
    system_power_control->disallow_default_system_handlers();
    system_power_control->allow_default_system_handlers();

    expect_no_inhibitions();
}

TEST_F(ALogindSystemPowerControl, notifies_of_system_resume)
{
    std::atomic<bool> system_resume{false};

    auto const system_resume_registration =
        system_power_control->register_system_resume_handler(
            [&] { system_resume = true; });

    fake_logind.emit_prepare_for_sleep(false);

    rt::spin_wait_for_condition_or_timeout(
        [&] { return system_resume.load(); },
        default_timeout);

    EXPECT_THAT(system_resume, Eq(true));
    EXPECT_TRUE(fake_log.contains_line({"PrepareForSleep", "false"}));
}

TEST_F(ALogindSystemPowerControl, notifies_of_system_allow_suspend_at_startup)
{
    auto const block_inhibited = "shutdown:handle-power-key";
    use_logind_with_initial_block_inhibited(block_inhibited);

    EXPECT_THAT(suspend_block_notifications, StrEq("[allow]"));
    EXPECT_TRUE(fake_log.contains_line({"BlockInhibited", block_inhibited}));
}

TEST_F(ALogindSystemPowerControl, notifies_of_system_disallow_suspend_at_startup)
{
    auto const block_inhibited = "shutdown:sleep:handle-power-key";
    use_logind_with_initial_block_inhibited(block_inhibited);

    EXPECT_THAT(suspend_block_notifications, StrEq("[disallow]"));
    EXPECT_TRUE(fake_log.contains_line({"BlockInhibited", block_inhibited}));
}

TEST_F(ALogindSystemPowerControl, notifies_of_system_allow_suspend_once)
{
    auto const block_inhibited_disallow = "shutdown:handle-power-key:sleep";
    auto const block_inhibited_allow = "shutdown:handle-power-key:slee";

    use_logind_with_initial_block_inhibited(block_inhibited_disallow);

    fake_logind.set_block_inhibited(block_inhibited_allow);
    fake_logind.set_block_inhibited(block_inhibited_allow);
    fake_logind.set_block_inhibited(block_inhibited_allow);

    expect_suspend_block_notifications("[disallow][allow]");
    EXPECT_TRUE(fake_log.contains_line({"BlockInhibited", block_inhibited_allow}));
}

TEST_F(ALogindSystemPowerControl, notifies_of_system_disallow_suspend_once)
{
    auto const block_inhibited_disallow = "sleep:shutdown:handle-power-key";

    fake_logind.set_block_inhibited(block_inhibited_disallow);
    fake_logind.set_block_inhibited(block_inhibited_disallow);
    fake_logind.set_block_inhibited(block_inhibited_disallow);

    expect_suspend_block_notifications("[allow][disallow]");
    EXPECT_TRUE(fake_log.contains_line({"BlockInhibited", block_inhibited_disallow}));
}

TEST_F(ALogindSystemPowerControl, system_allow_and_disallow_suspend_notifications_use_same_id)
{
    auto const block_inhibited_disallow = "shutdown:handle-power-key:sleep";
    auto const block_inhibited_allow = "sl";

    fake_logind.set_block_inhibited(block_inhibited_disallow);

    expect_suspend_block_notifications("[allow][disallow]");

    fake_logind.set_block_inhibited(block_inhibited_allow);

    expect_suspend_block_notifications("[allow][disallow][allow]");

    std::lock_guard<std::mutex> lock{mutex};
    EXPECT_THAT(suspend_block_ids, Each(suspend_block_ids[0]));
}

TEST_F(ALogindSystemPowerControl, disallow_default_system_handlers_logs_inhibition)
{
    system_power_control->disallow_default_system_handlers();

    EXPECT_TRUE(fake_log.contains_line({"inhibit", "idle:handle-lid-switch"}));
    EXPECT_TRUE(fake_log.contains_line({"inhibit", "idle:handle-lid-switch", "done"}));
}

TEST_F(ALogindSystemPowerControl, allow_default_system_handlers_logs_inhibition_release)
{
    system_power_control->allow_default_system_handlers();

    EXPECT_TRUE(fake_log.contains_line({"releasing", "idle", "lid", "inhibition"}));
}

TEST_F(ALogindSystemPowerControl, power_off_is_logged)
{
    system_power_control->power_off();

    EXPECT_TRUE(fake_log.contains_line({"power_off"}));
    EXPECT_TRUE(fake_log.contains_line({"power_off", "done"}));
}

TEST_F(ALogindSystemPowerControl, suspend_is_logged)
{
    system_power_control->suspend();

    EXPECT_TRUE(fake_log.contains_line({"suspend"}));
    EXPECT_TRUE(fake_log.contains_line({"suspend", "done"}));
}
