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

#pragma once

#include "handler_registration.h"
#include "power_action.h"
#include "power_supply.h"

#include <functional>
#include <chrono>

#include <sys/types.h>

namespace repowerd
{

using SetInactivityBehaviorHandler = std::function<void(PowerAction, PowerSupply, std::chrono::milliseconds, pid_t)>;
using SetLidBehaviorHandler = std::function<void(PowerAction, PowerSupply, pid_t)>;
using SetCriticalPowerBehaviorHandler = std::function<void(PowerAction, pid_t)>;

class ClientSettings
{
public:
    virtual ~ClientSettings() = default;

    virtual void start_processing() = 0;

    virtual HandlerRegistration register_set_inactivity_behavior_handler(
        SetInactivityBehaviorHandler const& handler) = 0;

    virtual HandlerRegistration register_set_lid_behavior_handler(
        SetLidBehaviorHandler const& handler) = 0;

    virtual HandlerRegistration register_set_critical_power_behavior_handler(
        SetCriticalPowerBehaviorHandler const& handler) = 0;

protected:
    ClientSettings() = default;
    ClientSettings(ClientSettings const&) = default;
    ClientSettings& operator=(ClientSettings const&) = default;
};

}
