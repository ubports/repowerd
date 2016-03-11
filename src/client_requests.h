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

#include <functional>

namespace repowerd
{

enum class TurnOnDisplayTimeout{normal, reduced};
using TurnOnDisplayHandler = std::function<void(TurnOnDisplayTimeout)>;

class ClientRequests
{
public:
    virtual ~ClientRequests() = default;

    virtual void set_turn_on_display_handler(TurnOnDisplayHandler const& handler) = 0;
    virtual void clear_turn_on_display_handler() = 0;

protected:
    ClientRequests() = default;
    ClientRequests (ClientRequests const&) = default;
    ClientRequests& operator=(ClientRequests const&) = default;
};

}
