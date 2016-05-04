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
#include "unity_screen_power_state_change_reason.h"
#include "event_loop_handler_registration.h"
#include "wakeup_service.h"

#include "src/core/infinite_timeout.h"

namespace
{

auto const null_handler = []{};
auto const null_arg_handler = [](auto){};

int32_t reason_to_dbus_param(repowerd::DisplayPowerChangeReason reason)
{
    repowerd::UnityScreenPowerStateChangeReason unity_screen_reason{
        repowerd::UnityScreenPowerStateChangeReason::unknown};

    switch (reason)
    {
    case repowerd::DisplayPowerChangeReason::power_button:
        unity_screen_reason = repowerd::UnityScreenPowerStateChangeReason::power_key;
        break;

    case repowerd::DisplayPowerChangeReason::activity:
        unity_screen_reason = repowerd::UnityScreenPowerStateChangeReason::inactivity;
        break;

    case repowerd::DisplayPowerChangeReason::proximity:
        unity_screen_reason = repowerd::UnityScreenPowerStateChangeReason::proximity;
        break;

    case repowerd::DisplayPowerChangeReason::notification:
        unity_screen_reason = repowerd::UnityScreenPowerStateChangeReason::notification;
        break;

    case repowerd::DisplayPowerChangeReason::call:
        unity_screen_reason = repowerd::UnityScreenPowerStateChangeReason::unknown;
        break;

    case repowerd::DisplayPowerChangeReason::call_done:
        unity_screen_reason = repowerd::UnityScreenPowerStateChangeReason::call_done;
        break;

    default:
        break;
    };

    return static_cast<int32_t>(unity_screen_reason);
}

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

char const* const dbus_powerd_interface = "com.canonical.powerd";
char const* const dbus_powerd_path = "/com/canonical/powerd";
char const* const dbus_powerd_service_name = "com.canonical.powerd";

char const* const unity_powerd_service_introspection = R"(
<node>
  <interface name='com.canonical.powerd'>
    <method name='requestSysState'>
      <arg type='s' name='name' direction='in' />
      <arg type='i' name='state' direction='in' />
      <arg type='s' name='cookie' direction='out' />
    </method>
    <method name='clearSysState'>
      <arg type='s' name='cookie' direction='in' />
    </method>
    <method name='requestWakeup'>
      <arg type='s' name='name' direction='in' />
      <arg type='t' name='time' direction='in' />
      <arg type='s' name='cookie' direction='out' />
    </method>
    <method name='clearWakeup'>
      <arg type='s' name='cookie' direction='in' />
    </method>
    <method name='getBrightnessParams'>
      <!-- Returns dim, min, max, and default brighness and whether or not
           autobrightness is supported, in that order -->
      <arg type='(iiiib)' name='params' direction="out" />
    </method>
    <signal name='Wakeup'>
    </signal>
  </interface>
</node>)";

}

repowerd::UnityScreenService::UnityScreenService(
    std::shared_ptr<WakeupService> const& wakeup_service,
    DeviceConfig const& device_config,
    std::string const& dbus_bus_address)
    : wakeup_service{wakeup_service},
      dbus_connection{dbus_bus_address},
      disable_inactivity_timeout_handler{null_handler},
      enable_inactivity_timeout_handler{null_handler},
      set_inactivity_timeout_handler{null_arg_handler},
      disable_autobrightness_handler{null_handler},
      enable_autobrightness_handler{null_handler},
      set_normal_brightness_value_handler{null_arg_handler},
      notification_handler{null_handler},
      no_notification_handler{null_handler},
      started{false},
      next_keep_display_on_id{1},
      active_notifications{0},
      next_request_sys_state_id{1},
      brightness_params(BrightnessParams::from_device_config(device_config))
{
}

void repowerd::UnityScreenService::start_processing()
{
    if (started) return;

    unity_screen_handler_registration = dbus_event_loop.register_object_handler(
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

    name_owner_changed_handler_registration = dbus_event_loop.register_signal_handler(
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

    powerd_handler_registration = dbus_event_loop.register_object_handler(
        dbus_connection,
        dbus_powerd_path,
        unity_powerd_service_introspection,
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

    wakeup_handler_registration = wakeup_service->register_wakeup_handler(
        [this] (std::string const&)
        {
            dbus_event_loop.enqueue([this] { dbus_emit_Wakeup(); });
        });

    dbus_connection.request_name(dbus_screen_service_name);
    dbus_connection.request_name(dbus_powerd_service_name);

    started = true;
}

repowerd::HandlerRegistration
repowerd::UnityScreenService::register_enable_inactivity_timeout_handler(
    EnableInactivityTimeoutHandler const& handler)
{
    return EventLoopHandlerRegistration{
        dbus_event_loop,
        [this, &handler] { enable_inactivity_timeout_handler = handler; },
        [this] { enable_inactivity_timeout_handler = null_handler; }};
}

repowerd::HandlerRegistration
repowerd::UnityScreenService::register_disable_inactivity_timeout_handler(
    DisableInactivityTimeoutHandler const& handler)
{
    return EventLoopHandlerRegistration{
        dbus_event_loop,
        [this, &handler] { disable_inactivity_timeout_handler = handler; },
        [this] { disable_inactivity_timeout_handler = null_handler; }};
}

repowerd::HandlerRegistration
repowerd::UnityScreenService::register_set_inactivity_timeout_handler(
    SetInactivityTimeoutHandler const& handler)
{
    return EventLoopHandlerRegistration{
        dbus_event_loop,
        [this, &handler] { set_inactivity_timeout_handler = handler; },
        [this] { set_inactivity_timeout_handler = null_arg_handler; }};
}

repowerd::HandlerRegistration
repowerd::UnityScreenService::register_disable_autobrightness_handler(
    DisableAutobrightnessHandler const& handler)
{
    return EventLoopHandlerRegistration{
        dbus_event_loop,
        [this, &handler] { disable_autobrightness_handler = handler; },
        [this] { disable_autobrightness_handler = null_handler; }};
}

repowerd::HandlerRegistration
repowerd::UnityScreenService::register_enable_autobrightness_handler(
    EnableAutobrightnessHandler const& handler)
{
    return EventLoopHandlerRegistration{
        dbus_event_loop,
        [this, &handler] { enable_autobrightness_handler = handler; },
        [this] { enable_autobrightness_handler = null_handler; }};
}

repowerd::HandlerRegistration
repowerd::UnityScreenService::register_set_normal_brightness_value_handler(
    SetNormalBrightnessValueHandler const& handler)
{
    return EventLoopHandlerRegistration{
        dbus_event_loop,
        [this, &handler] { set_normal_brightness_value_handler = handler; },
        [this] { set_normal_brightness_value_handler = null_arg_handler; }};
}

repowerd::HandlerRegistration
repowerd::UnityScreenService::register_notification_handler(
    NotificationHandler const& handler)
{
    return EventLoopHandlerRegistration{
        dbus_event_loop,
        [this, &handler] { notification_handler = handler; },
        [this] { notification_handler = null_handler; }};
}

repowerd::HandlerRegistration
repowerd::UnityScreenService::register_no_notification_handler(
    NoNotificationHandler const& handler)
{
    return EventLoopHandlerRegistration{
        dbus_event_loop,
        [this, &handler] { no_notification_handler = handler; },
        [this] { no_notification_handler = null_handler; }};
}

void repowerd::UnityScreenService::notify_display_power_on(
    DisplayPowerChangeReason reason)
{
    int32_t const power_state_on = 1;
    int32_t const reason_param = reason_to_dbus_param(reason);

    dbus_emit_DisplayPowerStateChange(power_state_on, reason_param);
}

void repowerd::UnityScreenService::notify_display_power_off(
    DisplayPowerChangeReason reason)
{
    int32_t const power_state_off = 0;
    int32_t const reason_param = reason_to_dbus_param(reason);

    dbus_emit_DisplayPowerStateChange(power_state_off, reason_param);
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
    else if (method_name == "requestSysState")
    {
        char const* name{""};
        int32_t state{-1};
        g_variant_get(parameters, "(&si)", &name, &state);

        try
        {
            auto const cookie = dbus_requestSysState(sender, name, state);
            g_dbus_method_invocation_return_value(
                invocation, g_variant_new("(s)", cookie.c_str()));
        }
        catch (std::exception const& e)
        {
            g_dbus_method_invocation_return_error_literal(
                invocation, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS, e.what());
        }
    }
    else if (method_name == "clearSysState")
    {
        char const* cookie{""};
        g_variant_get(parameters, "(&s)", &cookie);

        dbus_clearSysState(sender, cookie);

        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else if (method_name == "requestWakeup")
    {
        char const* name{""};
        uint64_t time{0};
        g_variant_get(parameters, "(&st)", &name, &time);

        auto const cookie = dbus_requestWakeup(name, time);

        g_dbus_method_invocation_return_value(
            invocation, g_variant_new("(s)", cookie.c_str()));
    }
    else if (method_name == "clearWakeup")
    {
        char const* cookie{""};
        g_variant_get(parameters, "(&s)", &cookie);

        dbus_clearWakeup(cookie);

        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else if (method_name == "getBrightnessParams")
    {
        auto params = dbus_getBrightnessParams();

        g_dbus_method_invocation_return_value(
            invocation,
            g_variant_new("((iiiib))",
                params.dim_value,
                params.min_value,
                params.max_value,
                params.default_value,
                params.autobrightness_supported));
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

        if (request_sys_state_ids.erase(name) > 0 &&
            request_sys_state_ids.empty())
        {
            // TODO: Allow suspend
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
    set_normal_brightness_value_handler(
        brightness/static_cast<float>(brightness_params.max_value));
}

void repowerd::UnityScreenService::dbus_setInactivityTimeouts(
    int32_t poweroff_timeout, int32_t /*dimmer_timeout*/)
{
    if (poweroff_timeout < 0) return;

    auto const timeout = poweroff_timeout == 0 ? repowerd::infinite_timeout : 
                                                 std::chrono::seconds{poweroff_timeout};
    set_inactivity_timeout_handler(timeout);
}

bool repowerd::UnityScreenService::dbus_setScreenPowerMode(
    std::string const& mode, int32_t reason)
{
    if (reason == static_cast<int32_t>(UnityScreenPowerStateChangeReason::notification) ||
        reason == static_cast<int32_t>(UnityScreenPowerStateChangeReason::snap_decision))
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

void repowerd::UnityScreenService::dbus_emit_DisplayPowerStateChange(
    int32_t power_state, int32_t reason)
{
    g_dbus_connection_emit_signal(
        dbus_connection,
        nullptr,
        dbus_screen_path,
        dbus_screen_interface,
        "DisplayPowerStateChange",
        g_variant_new("(ii)", power_state, reason),
        nullptr);
}

std::string repowerd::UnityScreenService::dbus_requestSysState(
    std::string const& sender,
    std::string const& /*name*/,
    int32_t state)
{
    int32_t const active_state{1};

    if (state != active_state)
        throw std::runtime_error{"Invalid state"};

    auto const id = next_request_sys_state_id++;
    request_sys_state_ids.emplace(sender, id);
    // TODO: disallow suspend
    return std::to_string(id);
}

void repowerd::UnityScreenService::dbus_clearSysState(
        std::string const& sender,
        std::string const& cookie)
{
    bool id_removed{false};

    int32_t id = 0;
    try { id = std::stoi(cookie); } catch(...) {}

    auto range = request_sys_state_ids.equal_range(sender);
    for (auto iter = range.first;
         iter != range.second;
         ++iter)
    {
        if (iter->second == id)
        {
            request_sys_state_ids.erase(iter);
            id_removed = true;
            break;
        }
    }

    if (id_removed && request_sys_state_ids.empty())
    {
        // TODO: Allow suspend
    }
}

std::string repowerd::UnityScreenService::dbus_requestWakeup(
    std::string const& /*name*/,
    uint64_t time)
{
    return wakeup_service->schedule_wakeup_at(std::chrono::system_clock::from_time_t(time));
}

void repowerd::UnityScreenService::dbus_clearWakeup(
    std::string const& cookie)
{
    wakeup_service->cancel_wakeup(cookie);
}

repowerd::BrightnessParams repowerd::UnityScreenService::dbus_getBrightnessParams()
{
    return brightness_params;
}

void repowerd::UnityScreenService::dbus_emit_Wakeup()
{
    g_dbus_connection_emit_signal(
        dbus_connection,
        nullptr,
        dbus_powerd_path,
        dbus_powerd_interface,
        "Wakeup",
        nullptr,
        nullptr);
}
