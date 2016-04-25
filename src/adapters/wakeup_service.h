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

#include "src/core/handler_registration.h"

#include <chrono>
#include <functional>
#include <string>

namespace repowerd
{

using WakeupHandler = std::function<void(std::string const&)>;

class WakeupService
{
public:
    virtual ~WakeupService() = default;

    virtual std::string schedule_wakeup_at(std::chrono::system_clock::time_point tp) = 0;
    virtual void cancel_wakeup(std::string const& cookie) = 0;
    virtual HandlerRegistration register_wakeup_handler(WakeupHandler const& handler) = 0;

protected:
    WakeupService() = default;
    WakeupService(WakeupService const&) = delete;
    WakeupService& operator=(WakeupService const&) = delete;
};

}
