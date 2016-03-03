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

#include "display_power_control.h"
#include "power_button.h"
#include "state_machine.h"

struct repowerd::Daemon::EventHandlerRegistration
{
    EventHandlerRegistration(std::function<void()> const& unregister)
        : unregister{std::make_unique<std::function<void()>>(unregister)}
    {
    }
    ~EventHandlerRegistration()
    {
        if (unregister) (*unregister)();
    }

    EventHandlerRegistration(EventHandlerRegistration&& reg) = default;

    std::unique_ptr<std::function<void()>> unregister;
};

repowerd::Daemon::Daemon(DaemonConfig& config)
    : display_power_control{config.the_display_power_control()},
      power_button{config.the_power_button()},
      state_machine{config.the_state_machine()},
      running{false}
{
}

void repowerd::Daemon::run()
{
    std::promise<void> started;
    run(started);
}

void repowerd::Daemon::run(std::promise<void>& started)
{
    auto const registrations = register_event_handlers();

    started.set_value();
    running = true;

    while (running)
    {
        auto const ev = dequeue_event();
        ev();
    }
}

void repowerd::Daemon::stop()
{
    enqueue_event([this] { running = false; });
}

std::vector<repowerd::Daemon::EventHandlerRegistration>
repowerd::Daemon::register_event_handlers()
{
    std::vector<EventHandlerRegistration> registrations;

    auto const id = power_button->register_power_button_handler(
        [this] (PowerButtonState state)
        { 
            if (state == PowerButtonState::pressed)
                enqueue_event([this] { state_machine->handle_power_key_press(); });
            else if (state == PowerButtonState::released)
                enqueue_event([this] { state_machine->handle_power_key_release(); } );
        });

    registrations.push_back(
        EventHandlerRegistration{
            [this, id]{ power_button->unregister_power_button_handler(id); }});

    return registrations;
}

void repowerd::Daemon::enqueue_event(Event const& event)
{
    std::lock_guard<std::mutex> lock{event_queue_mutex};

    event_queue.push_back(event);
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
