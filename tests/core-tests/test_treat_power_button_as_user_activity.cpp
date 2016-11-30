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
#include "acceptance_test.h"
#include "fake_state_machine_options.h"
#include "fake_shared.h"

#include <gtest/gtest.h>

namespace rt = repowerd::test;
using namespace testing;
using namespace std::chrono_literals;

namespace
{

struct StateMachineOptionsWithChangeDisplayState : rt::FakeStateMachineOptions
{
    bool treat_power_button_as_user_activity() const override
    {
        return treat_power_button_as_user_activity_;
    };

    void set_treat_power_button_as_user_activity(bool b)
    {
        treat_power_button_as_user_activity_ = b;
    }

    bool treat_power_button_as_user_activity_ = true;
};

struct DaemonConfigWithChangeDisplayState : rt::DaemonConfig
{
    std::shared_ptr<repowerd::StateMachineOptions> the_state_machine_options() override
    {
        if (!state_machine_options)
        {
            state_machine_options =
                std::make_shared<StateMachineOptionsWithChangeDisplayState>();
        }

        return state_machine_options;
    }

    std::shared_ptr<StateMachineOptionsWithChangeDisplayState> state_machine_options;
};
}

TEST(ATreatPowerButtonAsUserActivityOption,
     if_enabled_treats_power_button_as_user_activity)
{
    DaemonConfigWithChangeDisplayState config;
    config.the_state_machine_options();
    config.state_machine_options->set_treat_power_button_as_user_activity(true);

    rt::AcceptanceTestBase test{rt::fake_shared(config)};
    test.run_daemon();

    test.expect_display_turns_on();

    test.press_power_button();
    test.release_power_button();

    test.verify_expectations();

    test.expect_no_display_power_change();

    test.press_power_button();
    test.release_power_button();
    test.advance_time_by(test.user_inactivity_normal_display_off_timeout - 1ms);

    test.verify_expectations();

    test.expect_display_turns_off();
    test.advance_time_by(1ms);
}

TEST(ATreatPowerButtonAsUserActivityOption,
     if_enabled_ignores_proximity_when_turning_display_on)
{
    DaemonConfigWithChangeDisplayState config;
    config.the_state_machine_options();
    config.state_machine_options->set_treat_power_button_as_user_activity(true);

    rt::AcceptanceTestBase test{rt::fake_shared(config)};
    test.run_daemon();

    test.set_proximity_state_near();
    test.expect_display_turns_on();

    test.press_power_button();
    test.release_power_button();
}

TEST(ATreatPowerButtonAsUserActivityOption,
     if_disabled_does_not_treat_power_button_as_user_activity)
{
    DaemonConfigWithChangeDisplayState config;
    config.the_state_machine_options();
    config.state_machine_options->set_treat_power_button_as_user_activity(false);

    rt::AcceptanceTestBase test{rt::fake_shared(config)};
    test.run_daemon();

    test.expect_display_turns_on();

    test.press_power_button();
    test.release_power_button();

    test.verify_expectations();

    test.expect_display_turns_off();

    test.press_power_button();
    test.release_power_button();
}
