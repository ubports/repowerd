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
#include "daemon_config.h"
#include "fake_state_machine_options.h"
#include "fake_shared.h"

#include <gtest/gtest.h>

namespace rt = repowerd::test;

namespace
{

struct StateMachineOptionsWithTurnOnDisplayAtStartup : rt::FakeStateMachineOptions
{
    bool turn_on_display_at_startup() const override { return turn_on_display_at_startup_; };
    void set_turn_on_display_at_startup(bool b) { turn_on_display_at_startup_ = b; }

    bool turn_on_display_at_startup_ = true;
};

struct DaemonConfigWithTurnOnDisplayAtStartup : rt::DaemonConfig
{
    std::shared_ptr<repowerd::StateMachineOptions> the_state_machine_options() override
    {
        if (!state_machine_options)
        {
            state_machine_options =
                std::make_shared<StateMachineOptionsWithTurnOnDisplayAtStartup>();
        }

        return state_machine_options;
    }

    std::shared_ptr<StateMachineOptionsWithTurnOnDisplayAtStartup> state_machine_options;
};

struct ATurnOnDisplayAtStartupOption : testing::Test
{
    DaemonConfigWithTurnOnDisplayAtStartup config;
};

}

TEST_F(ATurnOnDisplayAtStartupOption, if_enabled_turns_on_display_at_startup)
{
    config.the_state_machine_options();
    config.state_machine_options->set_turn_on_display_at_startup(true);

    rt::AcceptanceTestBase test{rt::fake_shared(config)};

    test.expect_display_turns_on();

    test.run_daemon();
}

TEST_F(ATurnOnDisplayAtStartupOption, if_disabled_does_not_turn_on_display_at_startup)
{
    config.the_state_machine_options();
    config.state_machine_options->set_turn_on_display_at_startup(false);

    rt::AcceptanceTestBase test{rt::fake_shared(config)};

    test.expect_no_display_power_change();
    test.expect_no_display_brightness_change();

    test.run_daemon();
}
