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

enum class UserActivityType{change_power_state, extend_power_state};
using UserActivityHandler = std::function<void(UserActivityType)>;

class UserActivity
{
public:
    virtual ~UserActivity() = default;

    virtual HandlerRegistration register_user_activity_handler(
        UserActivityHandler const& handler) = 0;

protected:
    UserActivity() = default;
    UserActivity(UserActivity const&) = delete;
    UserActivity& operator=(UserActivity const&) = delete;
};

}
