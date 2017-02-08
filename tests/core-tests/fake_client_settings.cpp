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

#include "fake_client_settings.h"

namespace rt = repowerd::test;

namespace
{
auto null_arg3_handler = [](auto,auto,auto){};
auto null_arg4_handler = [](auto,auto,auto,auto){};
}

rt::FakeClientSettings::FakeClientSettings()
    : set_inactivity_behavior_handler{null_arg4_handler},
      set_lid_behavior_handler{null_arg3_handler}
{
}

void rt::FakeClientSettings::start_processing()
{
    mock.start_processing();
}

repowerd::HandlerRegistration rt::FakeClientSettings::register_set_inactivity_behavior_handler(
    SetInactivityBehaviorHandler const& handler)
{
    mock.register_set_inactivity_behavior_handler(handler);
    set_inactivity_behavior_handler = handler;
    return HandlerRegistration{
        [this]
        {
            mock.unregister_set_inactivity_behavior_handler();
            set_inactivity_behavior_handler = null_arg4_handler;
        }};
}

repowerd::HandlerRegistration rt::FakeClientSettings::register_set_lid_behavior_handler(
    SetLidBehaviorHandler const& handler)
{
    mock.register_set_lid_behavior_handler(handler);
    set_lid_behavior_handler = handler;
    return HandlerRegistration{
        [this]
        {
            mock.unregister_set_lid_behavior_handler();
            set_lid_behavior_handler = null_arg3_handler;
        }};
}

void rt::FakeClientSettings::emit_set_inactivity_behavior(
    PowerAction power_action,
    PowerSupply power_supply,
    std::chrono::milliseconds timeout,
    pid_t pid)
{
    set_inactivity_behavior_handler(power_action, power_supply, timeout, pid);
}

void rt::FakeClientSettings::emit_set_lid_behavior(
    PowerAction power_action,
    PowerSupply power_supply,
    pid_t pid)
{
    set_lid_behavior_handler(power_action, power_supply, pid);
}
