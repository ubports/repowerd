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

#include "timerfd_wakeup_service.h"
#include "event_loop_handler_registration.h"
#include "src/core/log.h"

#include <sys/timerfd.h>

#include <system_error>

namespace
{

char const* const log_tag = "Timerfd";

auto null_handler = [](auto){};

timespec to_timespec(std::chrono::system_clock::time_point const& tp)
{
    auto d = tp.time_since_epoch();
    auto const sec = std::chrono::duration_cast<std::chrono::seconds>(d);

    timespec ts;
    ts.tv_sec = sec.count();
    ts.tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(d - sec).count();

    return ts;
}

}

repowerd::TimerfdWakeupService::TimerfdWakeupService(std::shared_ptr<Log> const& log)
    : timerfd_fd{timerfd_create(CLOCK_REALTIME_ALARM, TFD_CLOEXEC)},
      cookie_pool{1},
      wakeup_handler{null_handler},
      event_loop{"Wakeup"}
{
    if (timerfd_fd == -1) {
        log->log(log_tag, "Failed to create timerfd with CLOCK_REALTIME_ALARM, trying CLOCK_REALTIME PLEASE note this will not wake up the device from suspend!");
        timerfd_fd = timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC);
    }

    if (timerfd_fd == -1)
        throw std::system_error{errno, std::system_category(), "Failed to create timerfd"};

    event_loop.watch_fd(
        timerfd_fd,
        [this]
        {
            auto const wakeup = *wakeups.begin();

            cookie_pool.remove(std::stoull(wakeup.second));
            wakeups.erase(wakeups.begin());

            reset_timerfd();

            wakeup_handler(wakeup.second);
        });

    reset_timerfd();
}

std::string repowerd::TimerfdWakeupService::schedule_wakeup_at(
    std::chrono::system_clock::time_point tp)
{
    std::string cookie;

    event_loop.enqueue(
        [this,tp,&cookie]
        {
            cookie = std::to_string(cookie_pool.generate());
            wakeups.insert({tp, cookie});

            reset_timerfd();
        }).wait();

    return cookie;
}

void repowerd::TimerfdWakeupService::cancel_wakeup(std::string const& cookie)
{
    event_loop.enqueue(
        [this,&cookie]
        {
            for (auto iter = wakeups.begin(); iter != wakeups.end(); ++iter)
            {
                if (iter->second == cookie)
                {
                    cookie_pool.remove(std::stoull(cookie));
                    wakeups.erase(iter);
                    break;
                }
            }

            reset_timerfd();
        }).wait();
}

repowerd::HandlerRegistration repowerd::TimerfdWakeupService::register_wakeup_handler(
    WakeupHandler const& handler)
{
    return EventLoopHandlerRegistration{
        event_loop,
        [this, &handler] { wakeup_handler = handler; },
        [this] { wakeup_handler = null_handler; }};
}

unsigned int repowerd::TimerfdWakeupService::num_stored_elements()
{
    unsigned int elements = 0;

    event_loop.enqueue(
        [this,&elements]
        {
            elements = cookie_pool.size() + wakeups.size();
        }).wait();

    return elements;
}

void repowerd::TimerfdWakeupService::reset_timerfd()
{
    timespec next_wakeup;

    if (wakeups.empty())
        next_wakeup = {0, 0};
    else
        next_wakeup = to_timespec(wakeups.begin()->first);

    timespec const interval{0,0};
    itimerspec const spec{interval, next_wakeup};

    timerfd_settime(timerfd_fd, TFD_TIMER_ABSTIME, &spec, nullptr);
}
