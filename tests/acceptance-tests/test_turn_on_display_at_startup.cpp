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
#include "mock_display_power_control.h"
#include "mock_brightness_control.h"

#include "src/core/daemon.h"

#include <thread>

#include <gtest/gtest.h>

namespace rt = repowerd::test;

namespace
{
struct DaemonConfigWithTurnOnDisplayAtStartup : rt::DaemonConfig
{
    bool turn_on_display_at_startup() override { return true; }
};
}

TEST(ATurnOnDisplayAtStartupOption, turns_on_display_at_startup)
{
    DaemonConfigWithTurnOnDisplayAtStartup config;
    repowerd::Daemon daemon{config};

    EXPECT_CALL(*config.the_mock_display_power_control(), turn_on());
    EXPECT_CALL(*config.the_mock_brightness_control(), set_normal_brightness());

    std::thread daemon_thread{[&] { daemon.run(); }};

    daemon.flush();
    daemon.stop();
    daemon_thread.join();
}
