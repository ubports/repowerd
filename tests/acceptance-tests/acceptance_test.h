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

#pragma once

#include "src/daemon.h"
#include "daemon_config.h"

#include <chrono>
#include <thread>

#include <gtest/gtest.h>

namespace repowerd
{
namespace test
{

struct AcceptanceTest : testing::Test
{
    AcceptanceTest();
    ~AcceptanceTest();

    void expect_display_turns_off();
    void expect_display_turns_on();
    void expect_long_press_notification();
    void expect_no_display_power_change();
    void verify_expectations();

    void advance_time_by(std::chrono::milliseconds advance);
    void emit_proximity_state_far();
    void emit_proximity_state_near();
    void perform_user_activity_extending_power_state();
    void perform_user_activity_changing_power_state();
    void press_power_button();
    void release_power_button();
    void set_proximity_state_far();
    void set_proximity_state_near();
    void turn_off_display();
    void turn_on_display();

    DaemonConfig config;
    Daemon daemon{config};
    std::thread daemon_thread;
};

}
}
