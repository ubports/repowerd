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

#include "daemon_config.h"

#include <memory>
#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace repowerd
{

class Daemon
{
public:
    Daemon(DaemonConfig& config);

    void run();
    void stop();
    void flush();

private:
    struct EventHandlerRegistration;
    using Event = std::function<void()>;

    std::vector<EventHandlerRegistration> register_event_handlers();
    void enqueue_event(Event const& event);
    void enqueue_priority_event(Event const& event);
    Event dequeue_event();

    std::shared_ptr<DisplayPowerControl> const display_power_control;
    std::shared_ptr<PowerButton> const power_button;
    std::shared_ptr<StateMachine> const state_machine;
    std::shared_ptr<Timer> const timer;
    std::shared_ptr<UserActivity> const user_activity;

    bool running;

    std::mutex event_queue_mutex;
    std::condition_variable event_queue_cv;
    std::deque<Event> event_queue;
};

}
