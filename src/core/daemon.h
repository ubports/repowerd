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
#include "handler_registration.h"

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
    using Action = std::function<void()>;

    std::vector<HandlerRegistration> register_event_handlers();
    void start_event_processing();
    void enqueue_action(Action const& event);
    void enqueue_priority_action(Action const& event);
    Action dequeue_action();

    std::shared_ptr<BrightnessControl> const brightness_control;
    std::shared_ptr<ClientRequests> const client_requests;
    std::shared_ptr<NotificationService> const notification_service;
    std::shared_ptr<PowerButton> const power_button;
    std::shared_ptr<ProximitySensor> const proximity_sensor;
    std::shared_ptr<StateMachine> const state_machine;
    std::shared_ptr<Timer> const timer;
    std::shared_ptr<UserActivity> const user_activity;
    std::shared_ptr<VoiceCallService> const voice_call_service;

    bool running;

    std::mutex action_queue_mutex;
    std::condition_variable action_queue_cv;
    std::deque<Action> action_queue;
};

}
