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
#include "fake_power_button.h"
#include "mock_power_button_event_sink.h"
#include "fake_timer.h"
#include "fake_user_activity.h"

namespace rt = repowerd::test;

std::shared_ptr<repowerd::DisplayPowerControl> rt::DaemonConfig::the_display_power_control()
{
    return the_mock_display_power_control();
}

std::shared_ptr<repowerd::PowerButton> rt::DaemonConfig::the_power_button()
{
    return the_fake_power_button();
}

std::shared_ptr<repowerd::PowerButtonEventSink> rt::DaemonConfig::the_power_button_event_sink()
{
    return the_mock_power_button_event_sink();
}

std::shared_ptr<repowerd::Timer> rt::DaemonConfig::the_timer()
{
    return the_fake_timer();
}

std::shared_ptr<repowerd::UserActivity> rt::DaemonConfig::the_user_activity()
{
    return the_fake_user_activity();
}

std::shared_ptr<rt::MockDisplayPowerControl> rt::DaemonConfig::the_mock_display_power_control()
{
    if (!mock_display_power_control)
        mock_display_power_control = std::make_shared<rt::MockDisplayPowerControl>();

    return mock_display_power_control;
}

std::shared_ptr<rt::FakePowerButton> rt::DaemonConfig::the_fake_power_button()
{
    if (!fake_power_button)
        fake_power_button = std::make_shared<rt::FakePowerButton>();

    return fake_power_button;
}

std::shared_ptr<rt::MockPowerButtonEventSink>
rt::DaemonConfig::the_mock_power_button_event_sink()
{
    if (!mock_power_button_event_sink)
        mock_power_button_event_sink = std::make_shared<rt::MockPowerButtonEventSink>();

    return mock_power_button_event_sink;
}

std::shared_ptr<rt::FakeTimer> rt::DaemonConfig::the_fake_timer()
{
    if (!fake_timer)
        fake_timer = std::make_shared<rt::FakeTimer>();

    return fake_timer;
}

std::shared_ptr<rt::FakeUserActivity> rt::DaemonConfig::the_fake_user_activity()
{
    if (!fake_user_activity)
        fake_user_activity = std::make_shared<rt::FakeUserActivity>();

    return fake_user_activity;
}
