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

#include "default_daemon_config.h"
#include "default_state_machine.h"

#include "power_button.h"
#include "display_power_control.h"

namespace
{

struct NullPowerButton : repowerd::PowerButton
{
    repowerd::PowerButtonHandlerId register_power_button_handler(
        repowerd::PowerButtonHandler const&) override
    {
        return {};
    }

    void unregister_power_button_handler(repowerd::PowerButtonHandlerId) override
    {
    }
};

struct NullDisplayPowerControl : repowerd::DisplayPowerControl
{
    void turn_on() override {}
    void turn_off() override {}
};

}

std::shared_ptr<repowerd::StateMachine>
repowerd::DefaultDaemonConfig::the_state_machine()
{
    if (!state_machine)
        state_machine = std::make_shared<DefaultStateMachine>(*this);
    return state_machine;
}

std::shared_ptr<repowerd::PowerButton>
repowerd::DefaultDaemonConfig::the_power_button()
{
    if (!power_button)
        power_button = std::make_shared<NullPowerButton>();
    return power_button;

}

std::shared_ptr<repowerd::DisplayPowerControl>
repowerd::DefaultDaemonConfig::the_display_power_control()
{
    if (!display_power_control)
        display_power_control = std::make_shared<NullDisplayPowerControl>();
    return display_power_control;
}
