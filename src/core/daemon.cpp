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

#include "daemon.h"

#include "brightness_control.h"
#include "client_requests.h"
#include "display_power_control.h"
#include "notification_service.h"
#include "null_state_machine.h"
#include "power_button.h"
#include "power_source.h"
#include "proximity_sensor.h"
#include "session_tracker.h"
#include "state_machine.h"
#include "state_machine_factory.h"
#include "timer.h"
#include "user_activity.h"
#include "voice_call_service.h"

#include <future>

namespace
{
std::string const null_session_id;
}
repowerd::Daemon::Session::Session(
    std::shared_ptr<StateMachine> const& state_machine)
    : state_machine{state_machine},
      state_event_adapter{*state_machine}
{
}

repowerd::Daemon::Daemon(DaemonConfig& config)
    : brightness_control{config.the_brightness_control()},
      client_requests{config.the_client_requests()},
      notification_service{config.the_notification_service()},
      power_button{config.the_power_button()},
      power_source{config.the_power_source()},
      proximity_sensor{config.the_proximity_sensor()},
      session_tracker{config.the_session_tracker()},
      state_machine_factory{config.the_state_machine_factory()},
      timer{config.the_timer()},
      user_activity{config.the_user_activity()},
      voice_call_service{config.the_voice_call_service()},
      turn_on_display_at_startup{config.turn_on_display_at_startup()},
      running{false}
{
    sessions.emplace(null_session_id, Session{std::make_shared<NullStateMachine>()});
    active_session = &sessions.at(null_session_id);
}

void repowerd::Daemon::run()
{
    auto const registrations = register_event_handlers();
    start_event_processing();

    running = true;

    while (running)
    {
        auto const ev = dequeue_action();
        ev();
    }
}

void repowerd::Daemon::stop()
{
    enqueue_priority_action([this] { running = false; });
}

void repowerd::Daemon::flush()
{
    std::promise<void> flushed_promise;
    auto flushed_future = flushed_promise.get_future();

    enqueue_action([&flushed_promise] { flushed_promise.set_value(); });

    flushed_future.wait();
}

std::vector<repowerd::HandlerRegistration>
repowerd::Daemon::register_event_handlers()
{
    std::vector<HandlerRegistration> registrations;

    registrations.push_back(
        power_button->register_power_button_handler(
            [this] (PowerButtonState state)
            {
                if (state == PowerButtonState::pressed)
                    enqueue_action([this] { active_session->state_machine->handle_power_button_press(); });
                else if (state == PowerButtonState::released)
                    enqueue_action([this] { active_session->state_machine->handle_power_button_release(); } );
            }));

    registrations.push_back(
        timer->register_alarm_handler(
            [this] (AlarmId id)
            {
                enqueue_action([this, id] { active_session->state_machine->handle_alarm(id); });
            }));

    registrations.push_back(
        user_activity->register_user_activity_handler(
            [this] (UserActivityType type)
            {
                if (type == UserActivityType::change_power_state)
                {
                    enqueue_action(
                        [this] { active_session->state_machine->handle_user_activity_changing_power_state(); });
                }
                else if (type == UserActivityType::extend_power_state)
                {
                    enqueue_action(
                        [this] { active_session->state_machine->handle_user_activity_extending_power_state(); });
                }
            }));

    registrations.push_back(
        proximity_sensor->register_proximity_handler(
            [this] (ProximityState state)
            {
                if (state == ProximityState::far)
                {
                    enqueue_action(
                        [this] { active_session->state_machine->handle_proximity_far(); });
                }
                else if (state == ProximityState::near)
                {
                    enqueue_action(
                        [this] { active_session->state_machine->handle_proximity_near(); });
                }
            }));

    registrations.push_back(
        client_requests->register_enable_inactivity_timeout_handler(
            [this] (std::string const& id)
            {
                enqueue_action(
                    [this, id] { active_session->state_event_adapter.handle_enable_inactivity_timeout(id); });
            }));

    registrations.push_back(
        client_requests->register_disable_inactivity_timeout_handler(
            [this] (std::string const& id)
            {
                enqueue_action(
                    [this, id] { active_session->state_event_adapter.handle_disable_inactivity_timeout(id); });
            }));

    registrations.push_back(
        client_requests->register_set_inactivity_timeout_handler(
            [this] (std::chrono::milliseconds timeout)
            {
                enqueue_action(
                    [this,timeout] { active_session->state_machine->handle_set_inactivity_timeout(timeout); });
            }));

    registrations.push_back(
        notification_service->register_notification_handler(
            [this] (std::string const& id)
            {
                enqueue_action(
                    [this,id] { active_session->state_event_adapter.handle_notification(id); });
            }));

    registrations.push_back(
        notification_service->register_notification_done_handler(
            [this] (std::string const& id)
            {
                enqueue_action(
                    [this,id] { active_session->state_event_adapter.handle_notification_done(id); });
            }));

    registrations.push_back(
        voice_call_service->register_active_call_handler(
            [this]
            {
                enqueue_action(
                    [this] { active_session->state_machine->handle_active_call(); });
            }));

    registrations.push_back(
        voice_call_service->register_no_active_call_handler(
            [this]
            {
                enqueue_action(
                    [this] { active_session->state_machine->handle_no_active_call(); });
            }));

    registrations.push_back(
        client_requests->register_set_normal_brightness_value_handler(
            [this] (double value)
            {
                enqueue_action(
                    [this,value] { brightness_control->set_normal_brightness_value(value); });
            }));

    registrations.push_back(
        client_requests->register_disable_autobrightness_handler(
            [this]
            {
                enqueue_action(
                    [this] { brightness_control->disable_autobrightness(); });
            }));

    registrations.push_back(
        client_requests->register_enable_autobrightness_handler(
            [this]
            {
                enqueue_action(
                    [this] { brightness_control->enable_autobrightness(); });
            }));

    registrations.push_back(
        power_source->register_power_source_change_handler(
            [this]
            {
                enqueue_action(
                    [this] { active_session->state_machine->handle_power_source_change(); });
            }));

    registrations.push_back(
        power_source->register_power_source_critical_handler(
            [this]
            {
                enqueue_action(
                    [this] { active_session->state_machine->handle_power_source_critical(); });
            }));

    registrations.push_back(
        session_tracker->register_active_session_changed_handler(
            [this] (std::string const& session_id, SessionType session_type)
            {
                enqueue_action(
                    [this, session_id, session_type]
                    {
                        handle_session_activated(session_id, session_type);
                    });
            }));

    registrations.push_back(
        session_tracker->register_session_removed_handler(
            [this] (std::string const& session_id)
            {
                enqueue_action(
                    [this, session_id] { handle_session_removed(session_id); });
            }));

    return registrations;
}

void repowerd::Daemon::start_event_processing()
{
    client_requests->start_processing();
    notification_service->start_processing();
    power_button->start_processing();
    power_source->start_processing();
    user_activity->start_processing();
    session_tracker->start_processing();
    voice_call_service->start_processing();
}

void repowerd::Daemon::enqueue_action(Action const& action)
{
    std::lock_guard<std::mutex> lock{action_queue_mutex};

    action_queue.push_back(action);
    action_queue_cv.notify_one();
}

void repowerd::Daemon::enqueue_priority_action(Action const& action)
{
    std::lock_guard<std::mutex> lock{action_queue_mutex};

    action_queue.push_front(action);
    action_queue_cv.notify_one();
}

repowerd::Daemon::Action repowerd::Daemon::dequeue_action()
{
    std::unique_lock<std::mutex> lock{action_queue_mutex};

    action_queue_cv.wait(lock, [this] { return !action_queue.empty(); });
    auto ev = action_queue.front();
    action_queue.pop_front();
    return ev;
}

void repowerd::Daemon::handle_session_activated(
    std::string const& session_id, SessionType session_type)
{
    if (session_type == SessionType::RepowerdIncompatible)
    {
        active_session = &sessions.at(null_session_id);
    }
    else
    {
        auto iter = sessions.find(session_id);
        if (iter == sessions.end())
        {
            iter = sessions.emplace(
                session_id,
                Session{state_machine_factory->create_state_machine()}).first;

            if (turn_on_display_at_startup)
                iter->second.state_machine->handle_turn_on_display();
        }

        active_session = &iter->second;
    }
}

void repowerd::Daemon::handle_session_removed(
    std::string const& session_id)
{
    sessions.erase(session_id);
}
