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

#include "daemon_config.h"
#include "run_daemon.h"
#include "mock_display_power_control.h"
#include "mock_brightness_control.h"
#include "fake_log.h"
#include "fake_state_machine_options.h"

#include "src/core/daemon.h"

#include <thread>

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
}

TEST(ATurnOnDisplayAtStartupOption, if_enabled_turns_on_display_at_startup)
{
    DaemonConfigWithTurnOnDisplayAtStartup config;
    config.the_state_machine_options();
    config.state_machine_options->set_turn_on_display_at_startup(true);

    repowerd::Daemon daemon{config};

    EXPECT_CALL(*config.the_mock_display_power_control(), turn_on());
    EXPECT_CALL(*config.the_mock_brightness_control(), set_normal_brightness());

    auto daemon_thread = rt::run_daemon(daemon);

    daemon.stop();
    daemon_thread.join();
}

TEST(ATurnOnDisplayAtStartupOption, if_disabled_does_not_turn_on_display_at_startup)
{
    DaemonConfigWithTurnOnDisplayAtStartup config;
    config.the_state_machine_options();
    config.state_machine_options->set_turn_on_display_at_startup(false);

    repowerd::Daemon daemon{config};

    EXPECT_CALL(*config.the_mock_display_power_control(), turn_on()).Times(0);
    EXPECT_CALL(*config.the_mock_brightness_control(), set_normal_brightness()).Times(0);

    auto daemon_thread = rt::run_daemon(daemon);

    daemon.stop();
    daemon_thread.join();
}
