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

namespace rt = repowerd::test;

std::shared_ptr<repowerd::PowerButton> rt::DaemonConfig::the_power_button()
{
    return the_fake_power_button();
}

std::shared_ptr<repowerd::DisplayPowerControl> rt::DaemonConfig::the_display_power_control()
{
    return the_mock_display_power_control();
}

std::shared_ptr<rt::FakePowerButton> rt::DaemonConfig::the_fake_power_button()
{
    if (!fake_power_button)
        fake_power_button = std::make_shared<rt::FakePowerButton>();

    return fake_power_button;
}

std::shared_ptr<rt::MockDisplayPowerControl> rt::DaemonConfig::the_mock_display_power_control()
{
    if (!mock_display_power_control)
        mock_display_power_control = std::make_shared<rt::MockDisplayPowerControl>();

    return mock_display_power_control;
}

