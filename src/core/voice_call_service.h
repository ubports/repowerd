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

namespace repowerd
{

using ActiveCallHandler = std::function<void()>;
using NoActiveCallHandler = std::function<void()>;

class VoiceCallService
{
public:
    virtual ~VoiceCallService() = default;

    virtual HandlerRegistration register_active_call_handler(
        ActiveCallHandler const& handler) = 0;

    virtual HandlerRegistration register_no_active_call_handler(
        NoActiveCallHandler const& handler) = 0;

protected:
    VoiceCallService() = default;
    VoiceCallService (VoiceCallService const&) = default;
    VoiceCallService& operator=(VoiceCallService const&) = default;
};

}
