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

#include "client_requests.h"
#include "display_power_control.h"
#include "notification_service.h"
#include "power_button.h"
#include "proximity_sensor.h"
#include "state_machine.h"
#include "timer.h"
#include "user_activity.h"

#include <future>

repowerd::Daemon::Daemon(DaemonConfig& config)
    : client_requests{config.the_client_requests()},
      display_power_control{config.the_display_power_control()},
      notification_service{config.the_notification_service()},
      power_button{config.the_power_button()},
      proximity_sensor{config.the_proximity_sensor()},
      state_machine{config.the_state_machine()},
      timer{config.the_timer()},
      user_activity{config.the_user_activity()},
      running{false}
{
}

void repowerd::Daemon::run()
{
    auto const registrations = register_event_handlers();

    running = true;

    while (running)
    {
        auto const ev = dequeue_event();
        ev();
    }
}

void repowerd::Daemon::stop()
{
    enqueue_priority_event([this] { running = false; });
}

void repowerd::Daemon::flush()
{
    std::promise<void> flushed_promise;
    auto flushed_future = flushed_promise.get_future();

    enqueue_event([&flushed_promise] { flushed_promise.set_value(); });

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
                    enqueue_event([this] { state_machine->handle_power_button_press(); });
                else if (state == PowerButtonState::released)
                    enqueue_event([this] { state_machine->handle_power_button_release(); } );
            }));

    registrations.push_back(
        timer->register_alarm_handler(
            [this] (AlarmId id)
            {
                enqueue_event([this, id] { state_machine->handle_alarm(id); });
            }));

    registrations.push_back(
        user_activity->register_user_activity_handler(
            [this] (UserActivityType type)
            {
                if (type == UserActivityType::change_power_state)
                {
                    enqueue_event(
                        [this] { state_machine->handle_user_activity_changing_power_state(); });
                }
                else if (type == UserActivityType::extend_power_state)
                {
                    enqueue_event(
                        [this] { state_machine->handle_user_activity_extending_power_state(); });
                }
            }));

    registrations.push_back(
        proximity_sensor->register_proximity_handler(
            [this] (ProximityState state)
            {
                if (state == ProximityState::far)
                {
                    enqueue_event(
                        [this] { state_machine->handle_proximity_far(); });
                }
                else if (state == ProximityState::near)
                {
                    enqueue_event(
                        [this] { state_machine->handle_proximity_near(); });
                }
            }));

    registrations.push_back(
        client_requests->register_enable_inactivity_timeout_handler(
            [this]
            {
                enqueue_event(
                    [this] { state_machine->handle_enable_inactivity_timeout(); });
            }));

    registrations.push_back(
        client_requests->register_disable_inactivity_timeout_handler(
            [this]
            {
                enqueue_event(
                    [this] { state_machine->handle_disable_inactivity_timeout(); });
            }));

    registrations.push_back(
        notification_service->register_notification_handler(
            [this]
            {
                enqueue_event(
                    [this] { state_machine->handle_notification(); });
            }));

    registrations.push_back(
        notification_service->register_all_notifications_done_handler(
            [this]
            {
                enqueue_event(
                    [this] { state_machine->handle_all_notifications_done(); });
            }));

    return registrations;
}

void repowerd::Daemon::enqueue_event(Event const& event)
{
    std::lock_guard<std::mutex> lock{event_queue_mutex};

    event_queue.push_back(event);
    event_queue_cv.notify_one();
}

void repowerd::Daemon::enqueue_priority_event(Event const& event)
{
    std::lock_guard<std::mutex> lock{event_queue_mutex};

    event_queue.push_front(event);
    event_queue_cv.notify_one();
}

repowerd::Daemon::Event repowerd::Daemon::dequeue_event()
{
    std::unique_lock<std::mutex> lock{event_queue_mutex};

    event_queue_cv.wait(lock, [this] { return !event_queue.empty(); });
    auto ev = event_queue.front();
    event_queue.pop_front();
    return ev;
}
