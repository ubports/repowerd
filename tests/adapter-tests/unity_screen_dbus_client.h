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

#include "dbus_client.h"

#include <functional>
#include <string>

namespace repowerd
{
namespace test
{

class UnityScreenDBusClient : public DBusClient
{
public:
    UnityScreenDBusClient(std::string const& address);

    DBusAsyncReplyString request_introspection();
    DBusAsyncReplyVoid request_set_user_brightness(int32_t brightness);
    DBusAsyncReplyVoid request_user_auto_brightness_enable(bool enabled);
    DBusAsyncReplyVoid request_set_inactivity_timeouts(
        int32_t poweroff_timeout, int32_t dimmer_timeout);
    DBusAsyncReplyVoid request_set_touch_visualization_enabled(bool enabled);
    DBusAsyncReplyBool request_set_screen_power_mode(
        std::string const& mode, int reason);

    DBusAsyncReplyInt request_keep_display_on();
    DBusAsyncReplyVoid request_remove_display_on_request(int id);
    DBusAsyncReply request_invalid_method();
    DBusAsyncReply request_method_with_invalid_arguments();
    DBusAsyncReply request_method_with_invalid_interface();
    char const* const unity_screen_interface = "com.canonical.Unity.Screen";

    struct DisplayPowerStateChangeParams
    {
        int32_t power_state;
        int32_t reason;
    };
    HandlerRegistration register_display_power_state_change_handler(
        std::function<void(DisplayPowerStateChangeParams)> const& f);
};

}
}
