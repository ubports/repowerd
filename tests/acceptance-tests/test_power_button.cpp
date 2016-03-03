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

#include "daemon_config.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>

namespace rt = repowerd::test;

namespace
{

struct APowerButton : testing::Test
{
    rt::DaemonConfig config;
    repowerd::Daemon daemon{config};
    std::thread daemon_thread;

    APowerButton()
    {
        daemon_thread = std::thread{ [this] { daemon.run(); }};
        daemon.flush();
    }

    ~APowerButton()
    {
        daemon.flush();
        daemon.stop();
        daemon_thread.join();
    }

    void turn_on_display()
    {
        EXPECT_CALL(*config.the_mock_display_power_control(), turn_on());
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

    void expect_display_turns_on()
    {
        EXPECT_CALL(*config.the_mock_display_power_control(), turn_on());
    }

    void expect_display_turns_off()
    {
        EXPECT_CALL(*config.the_mock_display_power_control(), turn_off());
    }

    void expect_no_display_power_change()
    {
        EXPECT_CALL(*config.the_mock_display_power_control(), turn_on()).Times(0);
        EXPECT_CALL(*config.the_mock_display_power_control(), turn_off()).Times(0);
    }
};

}

TEST_F(APowerButton, press_turns_on_display)
{
    expect_display_turns_on();

    press_power_button();
}

TEST_F(APowerButton, release_after_press_does_not_affect_display)
{
    expect_display_turns_on();

    press_power_button();
    release_power_button();
}

TEST_F(APowerButton, stray_release_is_ignored_when_display_is_off)
{
    expect_no_display_power_change();

    release_power_button();
}

TEST_F(APowerButton, stray_release_is_ignored_when_display_is_on)
{
    turn_on_display();

    expect_no_display_power_change();

    release_power_button();
}

TEST_F(APowerButton, press_does_nothing_if_display_is_already_on)
{
    turn_on_display();

    expect_no_display_power_change();

    press_power_button();
}

TEST_F(APowerButton, release_soon_after_press_turns_off_display_it_is_already_on)
{
    turn_on_display();

    expect_display_turns_off();

    press_power_button();
    release_power_button();
}
