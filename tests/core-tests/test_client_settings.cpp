/*
 * Copyright © 2017 Canonical Ltd.
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
#include "src/core/infinite_timeout.h"

#include <gtest/gtest.h>

#include <chrono>

namespace rt = repowerd::test;

using namespace std::chrono_literals;
using namespace testing;

namespace
{

std::string power_supply_to_str(repowerd::PowerSupply power_supply)
{
    if (power_supply == repowerd::PowerSupply::battery)
        return "battery";
    else if (power_supply == repowerd::PowerSupply::line_power)
        return "line_power";

    return "unknown";
}

repowerd::PowerSupply other(repowerd::PowerSupply power_supply)
{
    if (power_supply == repowerd::PowerSupply::battery)
        return repowerd::PowerSupply::line_power;
    else if (power_supply == repowerd::PowerSupply::line_power)
        return repowerd::PowerSupply::battery;
    throw std::runtime_error("Invalid power_supply value");
}

struct AClientSetting : rt::AcceptanceTest, WithParamInterface<repowerd::PowerSupply>
{
    AClientSetting()
        : display_off_timeout{user_inactivity_normal_display_off_timeout + 10s},
          suspend_timeout{user_inactivity_normal_suspend_timeout + 10s},
          power_supply{GetParam()}
    {
    }

    void apply_power_supply(repowerd::PowerSupply power_supply)
    {
        if (power_supply == repowerd::PowerSupply::battery)
        {
            use_battery_power();
            emit_power_source_change();
        }
        else if (power_supply == repowerd::PowerSupply::line_power)
        {
            use_line_power();
            emit_power_source_change();
        }
    }

    std::chrono::milliseconds const display_off_timeout;
    std::chrono::milliseconds const suspend_timeout;
    repowerd::PowerSupply const power_supply;
};

}

TEST_P(AClientSetting,
       for_display_off_timeout_is_used_on_matching_power_supply)
{
    client_setting_set_inactivity_behavior(
        repowerd::PowerAction::display_off,
        power_supply,
        display_off_timeout);

    apply_power_supply(power_supply);
    turn_off_display();

    turn_on_display();

    expect_no_display_power_change();
    advance_time_by(display_off_timeout - 1ms);
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(1ms);
}

TEST_P(AClientSetting,
       for_display_off_timeout_is_not_used_on_non_matching_power_supply)
{
    client_setting_set_inactivity_behavior(
        repowerd::PowerAction::display_off,
        power_supply,
        display_off_timeout);

    apply_power_supply(other(power_supply));
    turn_off_display();

    turn_on_display();

    expect_no_display_power_change();
    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(1ms);
}

TEST_P(AClientSetting,
       for_display_off_timeout_reschedules_timeout_on_matching_power_supply)
{
    apply_power_supply(power_supply);
    turn_off_display();

    turn_on_display();

    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);

    client_setting_set_inactivity_behavior(
        repowerd::PowerAction::display_off,
        power_supply,
        display_off_timeout);

    expect_no_display_power_change();
    advance_time_by(display_off_timeout - 1ms);
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(1ms);
}

TEST_P(AClientSetting,
       for_display_off_timeout_does_not_reschedule_timeout_on_non_matching_power_supply)
{
    apply_power_supply(power_supply);
    turn_off_display();

    turn_on_display();

    advance_time_by(user_inactivity_normal_display_off_timeout - 1ms);

    client_setting_set_inactivity_behavior(
        repowerd::PowerAction::display_off,
        other(power_supply),
        display_off_timeout);

    expect_display_turns_off();
    advance_time_by(1ms);
}

TEST_P(AClientSetting,
       for_display_off_timeout_is_not_used_when_switching_to_matching_power_supply)
{
    apply_power_supply(other(power_supply));
    turn_off_display();

    client_setting_set_inactivity_behavior(
        repowerd::PowerAction::display_off,
        power_supply,
        display_off_timeout);

    turn_on_display();

    advance_time_by(user_inactivity_normal_display_off_timeout - 20s);

    apply_power_supply(power_supply);

    expect_display_turns_off();
    advance_time_by(20s);
}

TEST_P(AClientSetting,
       for_display_off_timeout_is_used_when_performing_user_activity_after_switching_to_matching_power_supply)
{
    apply_power_supply(other(power_supply));
    turn_off_display();

    client_setting_set_inactivity_behavior(
        repowerd::PowerAction::display_off,
        power_supply,
        display_off_timeout);

    turn_on_display();

    advance_time_by(user_inactivity_normal_display_off_timeout - 20s);

    apply_power_supply(power_supply);

    perform_user_activity_extending_power_state();

    expect_no_display_power_change();
    advance_time_by(display_off_timeout - 1ms);
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(1ms);
}

TEST_P(AClientSetting,
       for_display_off_timeout_is_used_on_matching_power_supply_when_session_starts)
{
    auto const pid = rt::default_pid + 100;

    apply_power_supply(power_supply);
    turn_off_display();

    add_compatible_session("c0", pid);
    switch_to_session("c0");

    client_setting_set_inactivity_behavior(
        repowerd::PowerAction::display_off,
        power_supply,
        display_off_timeout,
        pid);

    turn_on_display();

    expect_no_display_power_change();
    advance_time_by(display_off_timeout - 1ms);
    verify_expectations();

    expect_display_turns_off();
    advance_time_by(1ms);
}

TEST_P(AClientSetting, for_display_off_inactivity_behavior_is_logged)
{
    client_setting_set_inactivity_behavior(
        repowerd::PowerAction::display_off,
        power_supply,
        display_off_timeout);

    auto const timeout_str = std::to_string(display_off_timeout.count());

    EXPECT_TRUE(
        log_contains_line({
            "set_inactivity_behavior",
            "display_off",
            power_supply_to_str(power_supply),
            timeout_str}));
}

TEST_P(AClientSetting,
       for_suspend_timeout_is_used_on_matching_power_supply)
{
    apply_power_supply(power_supply);
    turn_off_display();

    client_setting_set_inactivity_behavior(
        repowerd::PowerAction::suspend,
        power_supply,
        suspend_timeout);

    turn_on_display();

    expect_display_turns_off();
    advance_time_by(user_inactivity_normal_display_off_timeout);
    verify_expectations();

    expect_no_system_power_change();
    advance_time_by(
        suspend_timeout - user_inactivity_normal_display_off_timeout - 1ms);
    verify_expectations();

    expect_system_suspends_when_allowed("inactivity");
    advance_time_by(1ms);
}

TEST_P(AClientSetting,
       for_infinite_suspend_timeout_disables_suspend_on_matching_power_supply)
{
    apply_power_supply(power_supply);
    turn_off_display();

    client_setting_set_inactivity_behavior(
        repowerd::PowerAction::suspend,
        power_supply,
        repowerd::infinite_timeout);

    turn_on_display();

    expect_display_turns_off();
    expect_no_system_power_change();
    advance_time_by(user_inactivity_normal_display_off_timeout);
    verify_expectations();

    expect_no_system_power_change();
    advance_time_by(500h);
    verify_expectations();
}

TEST_P(AClientSetting,
       for_infinite_suspend_timeout_is_used_when_changing_to_matching_power_supply)
{
    apply_power_supply(other(power_supply));
    turn_off_display();

    client_setting_set_inactivity_behavior(
        repowerd::PowerAction::suspend,
        power_supply,
        repowerd::infinite_timeout);

    turn_on_display();

    apply_power_supply(power_supply);

    expect_display_turns_off();
    expect_no_system_power_change();
    advance_time_by(user_inactivity_normal_display_off_timeout);
    verify_expectations();

    expect_no_system_power_change();
    advance_time_by(500h);
    verify_expectations();
}

TEST_P(AClientSetting,
       for_suspend_timeout_is_used_on_matching_power_supply_when_session_starts)
{
    auto const pid = rt::default_pid + 100;

    apply_power_supply(power_supply);
    turn_off_display();

    add_compatible_session("c0", pid);
    switch_to_session("c0");

    client_setting_set_inactivity_behavior(
        repowerd::PowerAction::suspend,
        power_supply,
        suspend_timeout,
        pid);

    turn_on_display();

    expect_display_turns_off();
    advance_time_by(user_inactivity_normal_display_off_timeout);
    verify_expectations();

    expect_no_system_power_change();
    advance_time_by(
        suspend_timeout - user_inactivity_normal_display_off_timeout - 1ms);
    verify_expectations();

    expect_system_suspends_when_allowed("inactivity");
    advance_time_by(1ms);
}

TEST_P(AClientSetting, for_unsupported_action_in_inactivity_behavior_is_ignored)
{
    auto const timeout = 10s;

    apply_power_supply(power_supply);
    turn_off_display();

    client_setting_set_inactivity_behavior(
        repowerd::PowerAction::none,
        power_supply,
        timeout);

    turn_on_display();

    expect_no_system_power_change();
    expect_no_display_power_change();
    advance_time_by(timeout);
}

TEST_P(AClientSetting, for_suspend_inactivity_behavior_is_logged)
{
    client_setting_set_inactivity_behavior(
        repowerd::PowerAction::suspend,
        power_supply,
        display_off_timeout);

    auto const timeout_str = std::to_string(display_off_timeout.count());

    EXPECT_TRUE(
        log_contains_line({
            "set_inactivity_behavior",
            "suspend",
            power_supply_to_str(power_supply),
            timeout_str}));
}

TEST_P(AClientSetting,
       for_suspend_lid_behavior_is_used_on_matching_power_supply)
{
    apply_power_supply(power_supply);
    turn_off_display();

    client_setting_set_lid_behavior(
        repowerd::PowerAction::suspend,
        power_supply);

    turn_on_display();

    expect_system_suspends_when_allowed("lid");
    close_lid();
}

TEST_P(AClientSetting, for_suspend_lid_behavior_is_logged)
{
    client_setting_set_lid_behavior(
        repowerd::PowerAction::suspend,
        power_supply);

    EXPECT_TRUE(
        log_contains_line({
            "set_lid_behavior",
            "suspend",
            power_supply_to_str(power_supply)}));
}

TEST_P(AClientSetting,
       for_none_lid_behavior_is_used_on_matching_power_supply)
{
    apply_power_supply(power_supply);
    turn_off_display();

    client_setting_set_lid_behavior(
        repowerd::PowerAction::none,
        power_supply);

    turn_on_display();

    expect_no_system_power_change();
    expect_display_turns_off();
    close_lid();
}

TEST_P(AClientSetting,
       for_none_lid_behavior_is_not_used_on_non_matching_power_supply)
{
    apply_power_supply(power_supply);
    turn_off_display();

    client_setting_set_lid_behavior(
        repowerd::PowerAction::none,
        power_supply);

    apply_power_supply(other(power_supply));
    turn_off_display();

    turn_on_display();

    expect_system_suspends_when_allowed("lid");
    close_lid();
}

TEST_P(AClientSetting,
       for_none_lid_behavior_is_used_on_matching_power_supply_when_session_starts)
{
    auto const pid = rt::default_pid + 100;

    apply_power_supply(power_supply);
    turn_off_display();

    add_compatible_session("c0", pid);
    switch_to_session("c0");

    client_setting_set_lid_behavior(
        repowerd::PowerAction::none,
        power_supply,
        pid);

    turn_on_display();

    expect_no_system_power_change();
    expect_display_turns_off();
    close_lid();
}

TEST_P(AClientSetting, for_none_lid_behavior_is_logged)
{
    client_setting_set_lid_behavior(
        repowerd::PowerAction::none,
        power_supply);

    EXPECT_TRUE(
        log_contains_line({
            "set_lid_behavior",
            "none",
            power_supply_to_str(power_supply)}));
}

TEST_P(AClientSetting,
       for_display_off_lid_behavior_is_used_on_matching_power_supply)
{
    apply_power_supply(power_supply);
    turn_off_display();

    client_setting_set_lid_behavior(
        repowerd::PowerAction::display_off,
        power_supply);

    turn_on_display();

    expect_no_system_power_change();
    expect_display_turns_off();
    close_lid();
}

TEST_P(AClientSetting,
       for_display_off_lid_behavior_is_not_used_on_non_matching_power_supply)
{
    apply_power_supply(power_supply);
    turn_off_display();

    client_setting_set_lid_behavior(
        repowerd::PowerAction::display_off,
        power_supply);

    apply_power_supply(other(power_supply));
    turn_off_display();

    turn_on_display();

    expect_system_suspends_when_allowed("lid");
    close_lid();
}

TEST_P(AClientSetting,
       for_display_off_lid_behavior_is_used_on_matching_power_supply_when_session_starts)
{
    auto const pid = rt::default_pid + 100;

    apply_power_supply(power_supply);
    turn_off_display();

    add_compatible_session("c0", pid);
    switch_to_session("c0");

    client_setting_set_lid_behavior(
        repowerd::PowerAction::display_off,
        power_supply,
        pid);

    turn_on_display();

    expect_no_system_power_change();
    expect_display_turns_off();
    close_lid();
}

TEST_P(AClientSetting, for_display_off_lid_behavior_is_logged)
{
    client_setting_set_lid_behavior(
        repowerd::PowerAction::display_off,
        power_supply);

    EXPECT_TRUE(
        log_contains_line({
            "set_lid_behavior",
            "display_off",
            power_supply_to_str(power_supply)}));
}

INSTANTIATE_TEST_CASE_P(WithPowerSupply,
                        AClientSetting,
                        Values(
                            repowerd::PowerSupply::battery,
                            repowerd::PowerSupply::line_power));
