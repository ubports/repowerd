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

#include "state_machine.h"
#include "daemon_config.h"

namespace repowerd
{

class DefaultStateMachine : public StateMachine
{
public:
    DefaultStateMachine(DaemonConfig& config);

    void handle_power_key_press() override;
    void handle_power_key_release() override;

private:
    std::shared_ptr<DisplayPowerControl> const display_power_control;
    bool is_display_on;
};

}
