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

#include "unity_screen_dbus_client.h"

#include <glib.h>

namespace rt = repowerd::test;

rt::UnityScreenDBusClient::UnityScreenDBusClient(std::string const& address)
    : rt::DBusClient{
        address,
        "com.canonical.Unity.Screen",
        "/com/canonical/Unity/Screen"}
{
}

rt::DBusAsyncReplyString rt::UnityScreenDBusClient::request_introspection()
{
    return invoke_with_reply<rt::DBusAsyncReplyString>(
        "org.freedesktop.DBus.Introspectable", "Introspect",
        nullptr);
}

rt::DBusAsyncReplyVoid rt::UnityScreenDBusClient::request_set_user_brightness(int32_t brightness)
{
    return invoke_with_reply<rt::DBusAsyncReplyVoid>(
        unity_screen_interface, "setUserBrightness",
        g_variant_new("(i)", brightness));
}

rt::DBusAsyncReplyVoid rt::UnityScreenDBusClient::request_user_auto_brightness_enable(bool enabled)
{
    gboolean const e = enabled ? TRUE : FALSE;
    return invoke_with_reply<rt::DBusAsyncReplyVoid>(
        unity_screen_interface, "userAutobrightnessEnable",
        g_variant_new("(b)", e));
}

rt::DBusAsyncReplyVoid rt::UnityScreenDBusClient::request_set_inactivity_timeouts(
    int32_t poweroff_timeout,
    int32_t dimmer_timeout)
{
    return invoke_with_reply<rt::DBusAsyncReplyVoid>(
        unity_screen_interface, "setInactivityTimeouts",
        g_variant_new("(ii)", poweroff_timeout, dimmer_timeout));
}

rt::DBusAsyncReplyVoid rt::UnityScreenDBusClient::request_set_touch_visualization_enabled(bool enabled)
{
    gboolean const e = enabled ? TRUE : FALSE;
    return invoke_with_reply<rt::DBusAsyncReplyVoid>(
        unity_screen_interface, "setTouchVisualizationEnabled",
        g_variant_new("(b)", e));
}

rt::DBusAsyncReplyBool rt::UnityScreenDBusClient::request_set_screen_power_mode(
    std::string const& mode, int reason)
{
    auto mode_cstr = mode.c_str();
    return invoke_with_reply<rt::DBusAsyncReplyBool>(
        unity_screen_interface, "setScreenPowerMode",
        g_variant_new("(si)", mode_cstr, reason));
}

rt::DBusAsyncReplyInt rt::UnityScreenDBusClient::request_keep_display_on()
{
    return invoke_with_reply<rt::DBusAsyncReplyInt>(
        unity_screen_interface, "keepDisplayOn", nullptr);
}

rt::DBusAsyncReplyVoid rt::UnityScreenDBusClient::request_remove_display_on_request(int id)
{
    return invoke_with_reply<rt::DBusAsyncReplyVoid>(
        unity_screen_interface, "removeDisplayOnRequest",
        g_variant_new("(i)", id));
}

rt::DBusAsyncReply rt::UnityScreenDBusClient::request_invalid_method()
{
    return invoke_with_reply<rt::DBusAsyncReply>(
        unity_screen_interface, "invalidMethod", nullptr);
}

rt::DBusAsyncReply rt::UnityScreenDBusClient::request_method_with_invalid_arguments()
{
    char const* const str = "abcd";
    return invoke_with_reply<rt::DBusAsyncReply>(
        unity_screen_interface, "setUserBrightness",
        g_variant_new("(s)", str));
}

rt::DBusAsyncReply rt::UnityScreenDBusClient::request_method_with_invalid_interface()
{
    int const brightness = 10;
    return invoke_with_reply<rt::DBusAsyncReply>(
        "com.invalid.Interface", "setUserBrightness",
        g_variant_new("(i)", brightness));
}

repowerd::HandlerRegistration
rt::UnityScreenDBusClient::register_display_power_state_change_handler(
    std::function<void(DisplayPowerStateChangeParams)> const& func)
{
    return event_loop.register_signal_handler(
        connection,
        nullptr,
        "com.canonical.Unity.Screen",
        "DisplayPowerStateChange",
        "/com/canonical/Unity/Screen",
        [func] (
            GDBusConnection* /*connection*/,
            gchar const* /*sender*/,
            gchar const* /*object_path*/,
            gchar const* /*interface_name*/,
            gchar const* /*signal_name*/,
            GVariant* parameters)
        {
            int32_t power_state{-1};
            int32_t reason{-1};
            g_variant_get(parameters, "(ii)", &power_state, &reason);
            func({power_state, reason});
        });
}
