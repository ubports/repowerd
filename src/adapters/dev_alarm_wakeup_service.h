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

#include "wakeup_service.h"
#include "fd.h"

#include <map>
#include <thread>
#include <mutex>

namespace repowerd
{
class Filesystem;

class DevAlarmWakeupService : public WakeupService
{
public:
    DevAlarmWakeupService(std::shared_ptr<Filesystem> const& filesystem);
    ~DevAlarmWakeupService();

    std::string schedule_wakeup_at(std::chrono::system_clock::time_point tp) override;
    void cancel_wakeup(std::string const& cookie) override;

    HandlerRegistration register_wakeup_handler(
        WakeupHandler const& handler) override;

private:
    void reset_hardware_alarm();

    std::shared_ptr<Filesystem> const filesystem;
    Fd const dev_alarm_fd;
    std::thread wakeup_thread;

    std::mutex wakeup_mutex;
    bool running;
    uint64_t next_cookie;
    WakeupHandler wakeup_handler;
    std::multimap<std::chrono::system_clock::time_point,std::string> wakeups;
};

}
