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

#include "src/daemon.h"

#include "mock_display_power_control.h"
#include "fake_power_button.h"
#include "fake_timer.h"

#include "daemon_config.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>

namespace rt = repowerd::test;

namespace
{

struct AUserActivity : testing::Test
{
    rt::DaemonConfig config;
    repowerd::Daemon daemon{config};
    std::thread daemon_thread;

    AUserActivity()
    {
        daemon_thread = std::thread{ [this] { daemon.run(); }};
        daemon.flush();
    }

    ~AUserActivity()
    {
        daemon.flush();
        daemon.stop();
        daemon_thread.join();
    }

    void expect_no_display_power_change()
    {
        EXPECT_CALL(*config.the_mock_display_power_control(), turn_on()).Times(0);
        EXPECT_CALL(*config.the_mock_display_power_control(), turn_off()).Times(0);
    }

    void turn_on_display()
    {
        EXPECT_CALL(*config.the_mock_display_power_control(), turn_on());
        press_power_button();
        release_power_button();
        daemon.flush();
        testing::Mock::VerifyAndClearExpectations(config.the_mock_display_power_control().get());
    }

    void turn_off_display()
    {
        EXPECT_CALL(*config.the_mock_display_power_control(), turn_off());
        press_power_button();
        release_power_button();
        daemon.flush();
        testing::Mock::VerifyAndClearExpectations(config.the_mock_display_power_control().get());
    }

    void press_power_button()
    {
        config.the_fake_power_button()->press();
    }

    void release_power_button()
    {
        config.the_fake_power_button()->release();
    }

    void advance_time_by(std::chrono::milliseconds advance)
    {
        daemon.flush();
        config.the_fake_timer()->advance_by(advance);
        daemon.flush();
    }

    void expect_display_turns_off()
    {
        EXPECT_CALL(*config.the_mock_display_power_control(), turn_off());
    }

    std::chrono::seconds display_off_timeout{60};
};

}

TEST_F(AUserActivity, not_performed_allows_display_to_turn_off)
{
    turn_on_display();

    expect_display_turns_off();
    advance_time_by(display_off_timeout);
}

TEST_F(AUserActivity, not_performed_has_no_effect_after_display_is_turned_off)
{
    turn_on_display();
    turn_off_display();

    expect_no_display_power_change();
    advance_time_by(display_off_timeout);
}
