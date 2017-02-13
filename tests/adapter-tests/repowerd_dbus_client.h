/*
 * Copyright © 2017 Canonical Ltd.
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

#include "dbus_client.h"

#include <string>

namespace repowerd
{
namespace test
{

class RepowerdDBusClient : public DBusClient
{
public:
    RepowerdDBusClient(std::string const& address);

    DBusAsyncReplyString request_introspection();
    DBusAsyncReplyVoid request_set_inactivity_behavior(
        std::string const& power_action,
        std::string const& power_supply,
        int32_t timeout);
    DBusAsyncReplyVoid request_set_lid_behavior(
        std::string const& power_action,
        std::string const& power_supply);
    DBusAsyncReplyVoid request_set_critical_power_behavior(
        std::string const& power_action);
};

}
}
