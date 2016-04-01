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

#include "unity_screen_service.h"
#include "scoped_g_error.h"

#include <future>

namespace
{
struct NullHandlerRegistration : repowerd::HandlerRegistration
{
    NullHandlerRegistration() : HandlerRegistration{[]{}} {}
};

enum class PowerStateChangeReason
{
    unknown = 0,
    inactivity = 1,
    power_key = 2,
    proximity = 3,
    notification = 4,
    snap_decision = 5,
    call_done = 6
};

char const* const dbus_screen_interface = "com.canonical.Unity.Screen";
char const* const dbus_screen_path = "/com/canonical/Unity/Screen";
char const* const dbus_screen_service_name = "com.canonical.Unity.Screen";

char const* const unity_screen_service_introspection = R"(<!DOCTYPE node PUBLIC '-//freedesktop//DTD D-BUS Object Introspection 1.0//EN' 'http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd'>
<node>
  <interface name='com.canonical.Unity.Screen'>
    <method name='setScreenPowerMode'>
      <arg type='b' direction='out'/>
      <arg name='mode' type='s' direction='in'/>
      <arg name='reason' type='i' direction='in'/>
    </method>
    <method name='keepDisplayOn'>
      <arg type='i' direction='out'/>
    </method>
    <method name='removeDisplayOnRequest'>
      <arg name='id' type='i' direction='in'/>
    </method>
    <method name='setUserBrightness'>
      <arg name='brightness' type='i' direction='in'/>
    </method>
    <method name='userAutobrightnessEnable'>
      <arg name='enable' type='b' direction='in'/>
    </method>
    <method name='setInactivityTimeouts'>
      <arg name='poweroff_timeout' type='i' direction='in'/>
      <arg name='dimmer_timeout' type='i' direction='in'/>
    </method>
    <method name='setTouchVisualizationEnabled'>
      <arg name='enabled' type='b' direction='in'/>
    </method>
  </interface>
</node>)";

}

repowerd::UnityScreenService::UnityScreenService(
    std::string const& dbus_bus_address)
    : dbus_connection{dbus_bus_address},
      next_keep_display_on_id{1},
      active_notifications{0}
{
    dbus_connection.request_name(dbus_screen_service_name);
    dbus_event_loop.register_object_handler(
        dbus_connection,
        dbus_screen_path,
        unity_screen_service_introspection,
        [this] (
            GDBusConnection* connection,
            gchar const* sender,
            gchar const* object_path,
            gchar const* interface_name,
            gchar const* method_name,
            GVariant* parameters,
            GDBusMethodInvocation* invocation)
        {
            dbus_method_call(
                connection, sender, object_path, interface_name,
                method_name, parameters, invocation);
        });

    dbus_event_loop.register_signal_handler(
        dbus_connection,
        "org.freedesktop.DBus",
        "org.freedesktop.DBus",
        "NameOwnerChanged",
        "/org/freedesktop/DBus",
        [this] (
            GDBusConnection* connection,
            gchar const* sender,
            gchar const* object_path,
            gchar const* interface_name,
            gchar const* signal_name,
            GVariant* parameters)
        {
            dbus_signal(
                connection, sender, object_path, interface_name,
                signal_name, parameters);
        });
}

repowerd::UnityScreenService::~UnityScreenService()
{
    dbus_event_loop.stop();
}

repowerd::HandlerRegistration
repowerd::UnityScreenService::register_enable_inactivity_timeout_handler(
    EnableInactivityTimeoutHandler const& handler)
{
    enable_inactivity_timeout_handler = handler;
    return NullHandlerRegistration{};
}

repowerd::HandlerRegistration
repowerd::UnityScreenService::register_disable_inactivity_timeout_handler(
    DisableInactivityTimeoutHandler const& handler)
{
    disable_inactivity_timeout_handler = handler;
    return NullHandlerRegistration{};
}

repowerd::HandlerRegistration
repowerd::UnityScreenService::register_set_inactivity_timeout_handler(
    SetInactivityTimeoutHandler const& handler)
{
    set_inactivity_timeout_handler = handler;
    return NullHandlerRegistration{};
}

repowerd::HandlerRegistration
repowerd::UnityScreenService::register_disable_autobrightness_handler(
    DisableAutobrightnessHandler const& handler)
{
    disable_autobrightness_handler = handler;
    return NullHandlerRegistration{};
}

repowerd::HandlerRegistration
repowerd::UnityScreenService::register_enable_autobrightness_handler(
    EnableAutobrightnessHandler const& handler)
{
    enable_autobrightness_handler = handler;
    return NullHandlerRegistration{};
}

repowerd::HandlerRegistration
repowerd::UnityScreenService::register_set_normal_brightness_value_handler(
    SetNormalBrightnessValueHandler const& handler)
{
    set_normal_brightness_value_handler = handler;
    return NullHandlerRegistration{};
}

repowerd::HandlerRegistration
repowerd::UnityScreenService::register_notification_handler(
    NotificationHandler const& handler)
{
    notification_handler = handler;
    return NullHandlerRegistration{};
}

repowerd::HandlerRegistration
repowerd::UnityScreenService::register_no_notification_handler(
    NoNotificationHandler const& handler)
{
    no_notification_handler = handler;
    return NullHandlerRegistration{};
}

void repowerd::UnityScreenService::dbus_method_call(
    GDBusConnection* /*connection*/,
    gchar const* sender_cstr,
    gchar const* /*object_path_cstr*/,
    gchar const* /*interface_name_cstr*/,
    gchar const* method_name_cstr,
    GVariant* parameters,
    GDBusMethodInvocation* invocation)
{
    std::string const sender{sender_cstr ? sender_cstr : ""};
    std::string const method_name{method_name_cstr ? method_name_cstr : ""};

    if (method_name == "keepDisplayOn")
    {
        auto const id = dbus_keepDisplayOn(sender);

        g_dbus_method_invocation_return_value(invocation, g_variant_new("(i)", id));
    }
    else if (method_name == "removeDisplayOnRequest")
    {
        int32_t id{-1};
        g_variant_get(parameters, "(i)", &id);

        dbus_removeDisplayOnRequest(sender, id);

        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else if (method_name == "setUserBrightness")
    {
        int32_t brightness{0};
        g_variant_get(parameters, "(i)", &brightness);

        dbus_setUserBrightness(brightness);

        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else if (method_name == "setInactivityTimeouts")
    {
        int32_t poweroff_timeout{-1};
        int32_t dimmer_timeout{-1};
        g_variant_get(parameters, "(ii)", &poweroff_timeout, &dimmer_timeout);

        dbus_setInactivityTimeouts(poweroff_timeout, dimmer_timeout);

        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else if (method_name == "userAutobrightnessEnable")
    {
        gboolean enable{FALSE};
        g_variant_get(parameters, "(b)", &enable);

        dbus_userAutobrightnessEnable(enable == TRUE);

        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else if (method_name == "setScreenPowerMode")
    {
        char const* mode{""};
        int32_t reason{-1};
        g_variant_get(parameters, "(&si)", &mode, &reason);

        auto const result = dbus_setScreenPowerMode(mode, reason);

        g_dbus_method_invocation_return_value(
            invocation,
            g_variant_new("(b)", result ? TRUE : FALSE));
    }
    else
    {
        g_dbus_method_invocation_return_error_literal(
            invocation, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED, "");
    }
}

void repowerd::UnityScreenService::dbus_signal(
    GDBusConnection* /*connection*/,
    gchar const* sender_cstr,
    gchar const* object_path_cstr,
    gchar const* interface_name_cstr,
    gchar const* signal_name_cstr,
    GVariant* parameters)
{
    std::string const sender{sender_cstr ? sender_cstr : ""};
    std::string const object_path{object_path_cstr ? object_path_cstr : ""};
    std::string const interface_name{interface_name_cstr ? interface_name_cstr : ""};
    std::string const signal_name{signal_name_cstr ? signal_name_cstr : ""};

    if (sender == "org.freedesktop.DBus" &&
        object_path == "/org/freedesktop/DBus" &&
        interface_name == "org.freedesktop.DBus" &&
        signal_name == "NameOwnerChanged")
    {
        char const* name = "";
        char const* old_owner = "";
        char const* new_owner = "";
        g_variant_get(parameters, "(&s&s&s)", &name, &old_owner, &new_owner);

        dbus_NameOwnerChanged(name, old_owner, new_owner);
    }
}

int32_t repowerd::UnityScreenService::dbus_keepDisplayOn(std::string const& sender)
{
    auto const id = next_keep_display_on_id++;
    keep_display_on_ids.emplace(sender, id);
    disable_inactivity_timeout_handler();
    return id;
}

void repowerd::UnityScreenService::dbus_removeDisplayOnRequest(
    std::string const& sender, int32_t id)
{
    bool id_removed{false};

    auto range = keep_display_on_ids.equal_range(sender);
    for (auto iter = range.first;
         iter != range.second;
         ++iter)
    {
        if (iter->second == id)
        {
            keep_display_on_ids.erase(iter);
            id_removed = true;
            break;
        }
    }

    if (id_removed && keep_display_on_ids.empty())
        enable_inactivity_timeout_handler();
}

void repowerd::UnityScreenService::dbus_NameOwnerChanged(
    std::string const& name,
    std::string const& old_owner,
    std::string const& new_owner)
{
    if (new_owner.empty() && old_owner == name)
    {
        // If the disconnected client had issued keepDisplayOn requests
        // and after removing them there are now no more requests left,
        // tell the screen we don't need to keep the display on.
        if (keep_display_on_ids.erase(name) > 0 &&
            keep_display_on_ids.empty())
        {
            enable_inactivity_timeout_handler();
        }
    }
}

void repowerd::UnityScreenService::dbus_userAutobrightnessEnable(bool enable)
{
    if (enable)
        enable_autobrightness_handler();
    else
        disable_autobrightness_handler();
}

void repowerd::UnityScreenService::dbus_setUserBrightness(int32_t brightness)
{
    set_normal_brightness_value_handler(brightness/255.0f);
}

void repowerd::UnityScreenService::dbus_setInactivityTimeouts(
    int32_t poweroff_timeout, int32_t /*dimmer_timeout*/)
{
    set_inactivity_timeout_handler(std::chrono::seconds{poweroff_timeout});
}

bool repowerd::UnityScreenService::dbus_setScreenPowerMode(
    std::string const& mode, int32_t reason)
{
    if (reason == static_cast<int32_t>(PowerStateChangeReason::notification) ||
        reason == static_cast<int32_t>(PowerStateChangeReason::snap_decision))
    {
        if (mode == "on")
        {
            ++active_notifications;
            notification_handler();
        }
        else if (mode == "off")
        {
            if (active_notifications > 0)
            {
                --active_notifications;
                if (active_notifications == 0)
                    no_notification_handler();
            }
        }
        return true;
    }

    return false;
}
