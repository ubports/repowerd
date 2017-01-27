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
#include "state_event_adapter.h"
#include "session_tracker.h"

#include <memory>
#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <unordered_map>
#include <string>

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
    struct Session
    {
        Session(std::shared_ptr<StateMachine> const& state_machine);
        std::shared_ptr<StateMachine> const state_machine;
        StateEventAdapter state_event_adapter;
    };

    using Action = std::function<void()>;
    using SessionAction = std::function<void(Session*)>;

    std::vector<HandlerRegistration> register_event_handlers();
    void start_event_processing();
    void enqueue_action(Action const& action);
    void enqueue_priority_action(Action const& action);
    void enqueue_action_to_active_session(SessionAction const& action);
    void enqueue_action_to_all_sessions(SessionAction const& action);
    void enqueue_action_to_sessions(
        std::vector<std::string> const& sessions,
        SessionAction const& action);
    void enqueue_action_to_sessions(
        std::function<std::vector<std::string>()> const& sessions_func,
        SessionAction const& action);
    Action dequeue_action();

    void handle_session_activated(std::string const&, repowerd::SessionType);
    void handle_session_removed(std::string const&);
    std::vector<std::string> sessions_for_pid(pid_t pid);
    void add_session_with_active_call(Session* session);
    std::vector<std::string> session_with_active_calls();

    std::shared_ptr<BrightnessControl> const brightness_control;
    std::shared_ptr<ClientRequests> const client_requests;
    std::shared_ptr<ClientSettings> const client_settings;
    std::shared_ptr<Lid> const lid;
    std::shared_ptr<NotificationService> const notification_service;
    std::shared_ptr<PowerButton> const power_button;
    std::shared_ptr<PowerSource> const power_source;
    std::shared_ptr<ProximitySensor> const proximity_sensor;
    std::shared_ptr<SessionTracker> const session_tracker;
    std::shared_ptr<StateMachineFactory> const state_machine_factory;
    std::shared_ptr<Timer> const timer;
    std::shared_ptr<UserActivity> const user_activity;
    std::shared_ptr<VoiceCallService> const voice_call_service;

    bool running;

    std::unordered_map<std::string,Session> sessions;
    std::vector<std::string> sessions_with_active_calls;
    Session* active_session;

    std::mutex action_queue_mutex;
    std::condition_variable action_queue_cv;
    std::deque<Action> action_queue;
};

}
