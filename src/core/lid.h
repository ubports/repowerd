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

enum class LidState { closed, open };
using LidHandler = std::function<void(LidState)>;

class Lid
{
public:
    virtual ~Lid() = default;

    virtual void start_processing() = 0;

    virtual HandlerRegistration register_lid_handler(
        LidHandler const& handler) = 0;

protected:
    Lid() = default;
    Lid(Lid const&) = default;
    Lid& operator=(Lid const&) = default;
};

}
