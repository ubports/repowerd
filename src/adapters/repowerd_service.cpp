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

#include "repowerd_service.h"
#include "event_loop_handler_registration.h"
#include "scoped_g_error.h"

#include "src/core/infinite_timeout.h"
#include "src/core/log.h"

#include <stdexcept>

namespace
{

char const* const log_tag = "RepowerdService";

auto const null_arg2_handler = [](auto,auto){};
auto const null_arg3_handler = [](auto,auto,auto){};
auto const null_arg4_handler = [](auto,auto,auto,auto){};

char const* const dbus_repowerd_path = "/com/canonical/repowerd";
char const* const dbus_repowerd_service_name = "com.canonical.repowerd";

char const* const repowerd_service_introspection = R"(
<node>
  <interface name='com.canonical.repowerd'>
    <method name='SetInactivityBehavior'>
      <arg type='s' name='action' direction='in' />
      <arg type='s' name='power_supply' direction='in' />
      <arg type='i' name='timeout_sec' direction='in' />
    </method>
    <method name='SetLidBehavior'>
      <arg type='s' name='action' direction='in' />
      <arg type='s' name='power_supply' direction='in' />
    </method>
    <method name='SetCriticalPowerBehavior'>
      <arg type='s' name='action' direction='in' />
    </method>
  </interface>
</node>)";

repowerd::PowerAction power_action_from_string(std::string const& str)
{
    if (str == "none")
        return repowerd::PowerAction::none;
    else if (str == "display-off")
        return repowerd::PowerAction::display_off;
    else if (str == "suspend")
        return repowerd::PowerAction::suspend;
    else if (str == "power-off")
        return repowerd::PowerAction::power_off;
    else
        throw std::invalid_argument{"Invalid power action: " + str};
}

repowerd::PowerSupply power_supply_from_string(std::string const& str)
{
    if (str == "battery")
        return repowerd::PowerSupply::battery;
    else if (str == "line-power")
        return repowerd::PowerSupply::line_power;
    else
        throw std::invalid_argument{"Invalid power supply: " + str};
}

}

repowerd::RepowerdService::RepowerdService(
    std::shared_ptr<Log> const& log,
    std::string const& dbus_bus_address)
    : log{log},
      dbus_connection{dbus_bus_address},
      dbus_event_loop{"RepowerdService"},
      set_inactivity_behavior_handler{null_arg4_handler},
      set_lid_behavior_handler{null_arg3_handler},
      set_critical_power_behavior_handler{null_arg2_handler}
{
}

void repowerd::RepowerdService::start_processing()
{
    repowerd_handler_registration = dbus_event_loop.register_object_handler(
        dbus_connection,
        dbus_repowerd_path,
        repowerd_service_introspection,
        [this] (
            GDBusConnection* connection,
            gchar const* sender,
            gchar const* object_path,
            gchar const* interface_name,
            gchar const* method_name,
            GVariant* parameters,
            GDBusMethodInvocation* invocation)
        {
            try
            {
                dbus_method_call(
                    connection, sender, object_path, interface_name,
                    method_name, parameters, invocation);
            }
            catch (std::invalid_argument const& e)
            {
                g_dbus_method_invocation_return_error_literal(
                    invocation, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS, e.what());
            }
            catch (std::exception const& e)
            {
                g_dbus_method_invocation_return_error_literal(
                    invocation, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, e.what());
            }
        });

    dbus_connection.request_name(dbus_repowerd_service_name);
}

repowerd::HandlerRegistration
repowerd::RepowerdService::register_set_inactivity_behavior_handler(
    SetInactivityBehaviorHandler const& handler)
{
    return EventLoopHandlerRegistration{
        dbus_event_loop,
        [this, &handler] { set_inactivity_behavior_handler = handler; },
        [this] { set_inactivity_behavior_handler = null_arg4_handler; }};
}

repowerd::HandlerRegistration
repowerd::RepowerdService::register_set_lid_behavior_handler(
    SetLidBehaviorHandler const& handler)
{
    return EventLoopHandlerRegistration{
        dbus_event_loop,
        [this, &handler] { set_lid_behavior_handler = handler; },
        [this] { set_lid_behavior_handler = null_arg3_handler; }};
}

repowerd::HandlerRegistration
repowerd::RepowerdService::register_set_critical_power_behavior_handler(
    SetCriticalPowerBehaviorHandler const& handler)
{
    return EventLoopHandlerRegistration{
        dbus_event_loop,
        [this, &handler] { set_critical_power_behavior_handler = handler; },
        [this] { set_critical_power_behavior_handler = null_arg2_handler; }};
}

void repowerd::RepowerdService::dbus_method_call(
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
    auto const pid = dbus_get_invocation_sender_pid(invocation);

    if (method_name == "SetInactivityBehavior")
    {
        char const* power_action{""};
        char const* power_supply{""};
        int32_t timeout{-1};
        g_variant_get(parameters, "(&s&si)", &power_action, &power_supply, &timeout);

        dbus_SetInactivityBehavior(sender, power_action, power_supply, timeout, pid);

        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else if (method_name == "SetLidBehavior")
    {
        char const* power_action{""};
        char const* power_supply{""};
        g_variant_get(parameters, "(&s&s)", &power_action, &power_supply);

        dbus_SetLidBehavior(sender, power_action, power_supply, pid);

        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else if (method_name == "SetCriticalPowerBehavior")
    {
        char const* power_action{""};
        g_variant_get(parameters, "(&s)", &power_action);

        dbus_SetCriticalPowerBehavior(sender, power_action, pid);

        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else
    {
        dbus_unknown_method(sender, method_name);

        g_dbus_method_invocation_return_error_literal(
            invocation, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED, "");
    }
}

void repowerd::RepowerdService::dbus_SetInactivityBehavior(
    std::string const& sender,
    std::string const& power_action_str,
    std::string const& power_supply_str,
    int32_t power_action_timeout,
    pid_t pid)
{
    log->log(log_tag, "dbus_SetInactivityBehavior(%s,%s,%s,%d)",
             sender.c_str(),
             power_action_str.c_str(),
             power_supply_str.c_str(),
             power_action_timeout);

    auto const timeout = power_action_timeout <= 0 ? repowerd::infinite_timeout :
                                                     std::chrono::seconds{power_action_timeout};

    auto const power_action = power_action_from_string(power_action_str);
    auto const power_supply = power_supply_from_string(power_supply_str);

    if (power_action != PowerAction::display_off &&
        power_action != PowerAction::suspend)
    {
        throw std::invalid_argument{"Invalid power action for inactivity: " + power_action_str};
    }

    set_inactivity_behavior_handler(power_action, power_supply, timeout, pid);
}

void repowerd::RepowerdService::dbus_SetLidBehavior(
    std::string const& sender,
    std::string const& power_action_str,
    std::string const& power_supply_str,
    pid_t pid)
{
    log->log(log_tag, "dbus_SetLidBehavior(%s,%s,%s)",
             sender.c_str(),
             power_action_str.c_str(),
             power_supply_str.c_str());

    auto const power_action = power_action_from_string(power_action_str);
    auto const power_supply = power_supply_from_string(power_supply_str);

    if (power_action != PowerAction::none &&
        power_action != PowerAction::suspend)
    {
        throw std::invalid_argument{"Invalid power action for lid: " + power_action_str};
    }

    set_lid_behavior_handler(power_action, power_supply, pid);
}

void repowerd::RepowerdService::dbus_SetCriticalPowerBehavior(
    std::string const& sender,
    std::string const& power_action_str,
    pid_t pid)
{
    log->log(log_tag, "dbus_SetCriticalPowerBehavior(%s,%s)",
             sender.c_str(),
             power_action_str.c_str());

    auto const power_action = power_action_from_string(power_action_str);

    if (power_action != PowerAction::suspend &&
        power_action != PowerAction::power_off)
    {
        throw std::invalid_argument{"Invalid power action for critical power: " + power_action_str};
    }

    set_critical_power_behavior_handler(power_action, pid);
}

void repowerd::RepowerdService::dbus_unknown_method(
    std::string const& sender, std::string const& name)
{
    log->log(log_tag, "dbus_unknown_method(%s,%s)", sender.c_str(), name.c_str());
}

pid_t repowerd::RepowerdService::dbus_get_invocation_sender_pid(
    GDBusMethodInvocation* invocation)
{
    int constexpr timeout = 1000;
    auto constexpr null_cancellable = nullptr;
    ScopedGError error;
    auto const sender = g_dbus_method_invocation_get_sender(invocation);

    auto const result = g_dbus_connection_call_sync(
        dbus_connection,
        "org.freedesktop.DBus",
        "/org/freedesktop/DBus",
        "org.freedesktop.DBus",
        "GetConnectionUnixProcessID",
        g_variant_new("(s)", sender),
        G_VARIANT_TYPE("(u)"),
        G_DBUS_CALL_FLAGS_NONE,
        timeout,
        null_cancellable,
        error);

    if (!result)
    {
        log->log(log_tag, "failed to get pid of '%s': %s",
                 sender, error.message_str().c_str());
        return -1;
    }

    guint pid;
    g_variant_get(result, "(u)", &pid);
    g_variant_unref(result);

    return pid;
}
