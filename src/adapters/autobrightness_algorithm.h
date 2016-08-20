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

using AutobrightnessHandler = std::function<void(double brightness)>;

class EventLoop;

class AutobrightnessAlgorithm
{
public:
    virtual ~AutobrightnessAlgorithm() = default;
    
    virtual bool init(EventLoop& event_loop) = 0;

    virtual void new_light_value(double light) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;

    virtual HandlerRegistration register_autobrightness_handler(
        AutobrightnessHandler const& handler) = 0;

protected:
    AutobrightnessAlgorithm() = default;
    AutobrightnessAlgorithm(AutobrightnessAlgorithm const&) = delete;
    AutobrightnessAlgorithm& operator=(AutobrightnessAlgorithm const&) = delete;
};

}
