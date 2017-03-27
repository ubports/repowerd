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

#include <string>
#include <functional>

namespace repowerd
{

using SystemResumeHandler = std::function<void()>;
using SystemAllowSuspendHandler = std::function<void(std::string const&)>;
using SystemDisallowSuspendHandler = std::function<void(std::string const&)>;

class SystemPowerControl
{
public:
    virtual ~SystemPowerControl() = default;

    virtual void start_processing() = 0;

    virtual HandlerRegistration register_system_resume_handler(
        SystemResumeHandler const& system_resume_handler) = 0;

    virtual HandlerRegistration register_system_allow_suspend_handler(
        SystemAllowSuspendHandler const& system_allow_suspend_handler) = 0;
    virtual HandlerRegistration register_system_disallow_suspend_handler(
        SystemDisallowSuspendHandler const& system_disallow_suspend_handler) = 0;

    virtual void allow_automatic_suspend(std::string const& id) = 0;
    virtual void disallow_automatic_suspend(std::string const& id) = 0;

    virtual void power_off() = 0;
    virtual void suspend() = 0;

    virtual void allow_default_system_handlers() = 0;
    virtual void disallow_default_system_handlers() = 0;

protected:
    SystemPowerControl() = default;
    SystemPowerControl (SystemPowerControl const&) = default;
    SystemPowerControl& operator=(SystemPowerControl const&) = default;
};

}
