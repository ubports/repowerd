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

#include <functional>
#include <thread>

namespace rt = repowerd::test;

namespace
{

struct APowerButton : testing::Test
{
    rt::DaemonConfig config;
    repowerd::Daemon daemon{config};
    std::promise<void> daemon_started_promise;
    std::thread daemon_thread;

    APowerButton()
    {
        auto daemon_started_future = daemon_started_promise.get_future();

        daemon_thread = std::thread{
            [this]
            {
                daemon.run(daemon_started_promise);
            }};

        daemon_started_future.wait();
    }

    ~APowerButton()
    {
        daemon.stop();
        daemon_thread.join();
    }

    void press_power_button()
    {
        config.the_fake_power_button()->press();
    }
};

}

TEST_F(APowerButton, press_turns_on_screen)
{
    EXPECT_CALL(*config.the_mock_display_power_control(), turn_on());

    press_power_button();
}
