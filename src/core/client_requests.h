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

#include "handler_registration.h"

#include <functional>
#include <chrono>
#include <string>

#include <sys/types.h>

namespace repowerd
{

using EnableInactivityTimeoutHandler = std::function<void(std::string const&, pid_t)>;
using DisableInactivityTimeoutHandler = std::function<void(std::string const&, pid_t)>;
using SetInactivityTimeoutHandler = std::function<void(std::chrono::milliseconds, pid_t)>;
using SetNormalBrightnessValueHandler = std::function<void(double, pid_t)>;
using EnableAutobrightnessHandler = std::function<void(pid_t)>;
using DisableAutobrightnessHandler = std::function<void(pid_t)>;

class ClientRequests
{
public:
    virtual ~ClientRequests() = default;

    virtual void start_processing() = 0;

    virtual HandlerRegistration register_enable_inactivity_timeout_handler(
        EnableInactivityTimeoutHandler const& handler) = 0;
    virtual HandlerRegistration register_disable_inactivity_timeout_handler(
        DisableInactivityTimeoutHandler const& handler) = 0;
    virtual HandlerRegistration register_set_inactivity_timeout_handler(
        SetInactivityTimeoutHandler const& handler) = 0;

    virtual HandlerRegistration register_set_normal_brightness_value_handler(
        SetNormalBrightnessValueHandler const& handler) = 0;
    virtual HandlerRegistration register_enable_autobrightness_handler(
        EnableAutobrightnessHandler const& handler) = 0;
    virtual HandlerRegistration register_disable_autobrightness_handler(
        DisableAutobrightnessHandler const& handler) = 0;

protected:
    ClientRequests() = default;
    ClientRequests (ClientRequests const&) = default;
    ClientRequests& operator=(ClientRequests const&) = default;
};

}
