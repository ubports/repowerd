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

#include "dev_alarm_wakeup_service.h"
#include "filesystem.h"

#include <android/linux/android_alarm.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <system_error>
#include <limits>

namespace
{
auto null_handler = [](auto){};

void ioctl_or_throw(
    repowerd::Filesystem& fs,
    int fd, unsigned long request, void* args,
    char const* error_msg)
{
    int retval;
    do
    {
        retval = fs.ioctl(fd, request, args);
    }
    while (retval < 0 && errno == EINTR);

    if (retval < 0)
        throw std::system_error{errno, std::system_category(), error_msg};
}
}

repowerd::DevAlarmWakeupService::DevAlarmWakeupService(
    std::shared_ptr<Filesystem> const& filesystem)
    : filesystem{filesystem},
      dev_alarm_fd{filesystem->open("/dev/alarm", O_RDWR)},
      running{true},
      next_cookie{1},
      wakeup_handler{null_handler}
{
    if (dev_alarm_fd == -1)
        throw std::system_error{errno, std::system_category(), "Failed to open /dev/alarm"};

    reset_hardware_alarm();

    wakeup_thread = std::thread(
        [this]
        {
            std::unique_lock<std::mutex> lock{wakeup_mutex};

            while (running)
            {
                lock.unlock();
                ioctl_or_throw(
                    *this->filesystem, dev_alarm_fd, ANDROID_ALARM_WAIT, nullptr,
                    "Failed to wait for alarm on /dev/alarm");
                lock.lock();
                if (running && !wakeups.empty())
                {
                    auto const wakeup = *wakeups.begin();
                    wakeups.erase(wakeups.begin());
                    reset_hardware_alarm();
                    auto const handler = wakeup_handler;
                    lock.unlock();
                    handler(wakeup.second);
                    lock.lock();
                }
            }
        });
}

repowerd::DevAlarmWakeupService::~DevAlarmWakeupService()
{
    {
        std::lock_guard<std::mutex> lock{wakeup_mutex};
        running = false;
        reset_hardware_alarm();
    }
    wakeup_thread.join();
}

std::string repowerd::DevAlarmWakeupService::schedule_wakeup_at(
    std::chrono::system_clock::time_point tp)
{
    std::lock_guard<std::mutex> lock{wakeup_mutex};

    auto const cookie = std::to_string(next_cookie++);
    wakeups.insert({tp, cookie});

    reset_hardware_alarm();
    return cookie;
}

void repowerd::DevAlarmWakeupService::cancel_wakeup(std::string const& cookie)
{
    std::lock_guard<std::mutex> lock{wakeup_mutex};

    for (auto iter = wakeups.begin(); iter != wakeups.end(); ++iter)
    {
        if (iter->second == cookie)
        {
            wakeups.erase(iter);
            break;
        }
    }

    reset_hardware_alarm();
}

repowerd::HandlerRegistration repowerd::DevAlarmWakeupService::register_wakeup_handler(
    WakeupHandler const& handler)
{
    std::lock_guard<std::mutex> lock{wakeup_mutex};
    wakeup_handler = handler;
    return repowerd::HandlerRegistration{
        [this]
        {
            std::lock_guard<std::mutex> lock{wakeup_mutex};
            wakeup_handler = null_handler;
        }};
}

timespec to_timespec(std::chrono::system_clock::time_point const& tp)
{
    auto d = tp.time_since_epoch();
    auto const sec = std::chrono::duration_cast<std::chrono::seconds>(d);

    timespec ts;
    ts.tv_sec = sec.count();
    ts.tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(d - sec).count();

    return ts;
}

void repowerd::DevAlarmWakeupService::reset_hardware_alarm()
{
    using std::chrono::system_clock;

    timespec next_wakeup;

    if (running)
    {
        if (wakeups.empty())
        {
            next_wakeup.tv_sec = std::numeric_limits<decltype(next_wakeup.tv_sec)>::max();
            next_wakeup.tv_nsec = std::numeric_limits<decltype(next_wakeup.tv_nsec)>::max();
        }
        else
        {
            next_wakeup = to_timespec(wakeups.begin()->first);
        }
    }
    else
    {
        next_wakeup = to_timespec(system_clock::now());
    }

    ioctl_or_throw(
        *filesystem, dev_alarm_fd, ANDROID_ALARM_SET(ANDROID_ALARM_RTC_WAKEUP), &next_wakeup,
        "Failed to set alarm on /dev/alarm");
}
