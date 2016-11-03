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

namespace repowerd
{

enum class SessionType { RepowerdCompatible, RepowerdIncompatible };
using ActiveSessionChangedHandler = std::function<void(std::string const&, SessionType)>;
using SessionRemovedHandler = std::function<void(std::string const&)>;

std::string const invalid_session_id;

class SessionTracker
{
public:
    virtual ~SessionTracker() = default;

    virtual void start_processing() = 0;

    virtual HandlerRegistration register_active_session_changed_handler(
        ActiveSessionChangedHandler const& handler) = 0;
    virtual HandlerRegistration register_session_removed_handler(
        SessionRemovedHandler const& handler) = 0;

protected:
    SessionTracker() = default;
    SessionTracker (SessionTracker const&) = default;
    SessionTracker& operator=(SessionTracker const&) = default;
};

}
