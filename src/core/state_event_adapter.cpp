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

#include "state_event_adapter.h"

repowerd::StateEventAdapter::StateEventAdapter(StateMachine& state_machine)
    : state_machine(state_machine)
{
}

void repowerd::StateEventAdapter::handle_enable_inactivity_timeout(
    std::string const& request_id)
{
    inactivity_timeout_disallowances.erase(request_id);

    if (inactivity_timeout_disallowances.empty())
        state_machine.handle_enable_inactivity_timeout();
}

void repowerd::StateEventAdapter::handle_disable_inactivity_timeout(
    std::string const& request_id)
{
    inactivity_timeout_disallowances.insert(request_id);

    state_machine.handle_disable_inactivity_timeout();
}

void repowerd::StateEventAdapter::handle_notification(
    std::string const& notification_id)
{
    active_notifications.insert(notification_id);

    state_machine.handle_notification();
}

void repowerd::StateEventAdapter::handle_notification_done(
    std::string const& notification_id)
{
    auto const removed = active_notifications.erase(notification_id) == 1;

    if (removed && active_notifications.empty())
        state_machine.handle_no_notification();
}

void repowerd::StateEventAdapter::handle_allow_suspend(
    std::string const& request_id)
{
    suspend_disallowances.erase(request_id);

    if (suspend_disallowances.empty())
        state_machine.handle_allow_suspend();
}

void repowerd::StateEventAdapter::handle_disallow_suspend(
    std::string const& request_id)
{
    suspend_disallowances.insert(request_id);

    state_machine.handle_disallow_suspend();
}
