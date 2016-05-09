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

#include <functional>

namespace repowerd
{

using LightHandler = std::function<void(double)>;

class LightSensor
{
public:
    virtual ~LightSensor() = default;

    virtual HandlerRegistration register_light_handler(
        LightHandler const& handler) = 0;

    virtual void enable_light_events() = 0;
    virtual void disable_light_events() = 0;

protected:
    LightSensor() = default;
    LightSensor (LightSensor const&) = default;
    LightSensor& operator=(LightSensor const&) = default;
};

}
