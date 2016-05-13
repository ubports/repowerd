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

#include "src/core/daemon.h"
#include "src/core/display_power_change_reason.h"
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
    void expect_display_dims();
    void expect_display_brightens();
    void expect_long_press_notification();
    void expect_no_display_power_change();
    void expect_no_display_brightness_change();
    void expect_display_power_off_notification(DisplayPowerChangeReason);
    void expect_display_power_on_notification(DisplayPowerChangeReason);
    void verify_expectations();

    void advance_time_by(std::chrono::milliseconds advance);
    void client_request_disable_inactivity_timeout();
    void client_request_enable_inactivity_timeout();
    void client_request_set_inactivity_timeout(std::chrono::milliseconds timeout);
    void emit_no_notification();
    void emit_notification();
    void emit_proximity_state_far();
    void emit_proximity_state_far_if_enabled();
    void emit_proximity_state_near();
    void emit_proximity_state_near_if_enabled();
    void emit_active_call();
    void emit_no_active_call();
    void perform_user_activity_extending_power_state();
    void perform_user_activity_changing_power_state();
    void press_power_button();
    void release_power_button();
    void set_proximity_state_far();
    void set_proximity_state_near();
    void turn_off_display();
    void turn_on_display();

    bool log_contains_line(std::vector<std::string> const& words);

    DaemonConfig config;
    Daemon daemon{config};
    std::thread daemon_thread;

    std::chrono::milliseconds const power_button_long_press_timeout;
    std::chrono::milliseconds const user_inactivity_normal_display_dim_duration;
    std::chrono::milliseconds const user_inactivity_normal_display_dim_timeout;
    std::chrono::milliseconds const user_inactivity_normal_display_off_timeout;
    std::chrono::milliseconds const user_inactivity_post_notification_display_off_timeout;
    std::chrono::milliseconds const user_inactivity_reduced_display_off_timeout;
    std::chrono::milliseconds const infinite_timeout;
};

}
}
