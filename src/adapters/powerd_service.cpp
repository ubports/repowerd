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

#include "powerd_service.h"
#include "unity_screen_service.h"
#include "brightness_params.h"

#include <hybris/properties/properties.h>

#include <stdexcept>

namespace
{

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
    <method name='getBrightnessParams'>
      <!-- Returns dim, min, max, and default brighness and whether or not
           autobrightness is supported, in that order -->
      <arg type='(iiiib)' name='params' direction="out" />
    </method>
  </interface>
</node>)";

}

repowerd::PowerdService::PowerdService(
    std::shared_ptr<UnityScreenService> const& unity_screen_service,
    DeviceConfig const& device_config,
    std::string const& dbus_bus_address)
    : unity_screen_service{unity_screen_service},
      dbus_connection{dbus_bus_address},
      brightness_params(BrightnessParams::from_device_config(device_config))
{
    dbus_connection.request_name(dbus_powerd_service_name);
    dbus_event_loop.register_object_handler(
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
}

repowerd::PowerdService::~PowerdService()
{
    dbus_event_loop.stop();
}

void repowerd::PowerdService::dbus_method_call(
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

    if (method_name == "requestSysState")
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

std::string repowerd::PowerdService::dbus_requestSysState(
    std::string const& sender,
    std::string const& /*name*/,
    int32_t state)
{
    int32_t const active_state{1};

    if (state != active_state)
        throw std::runtime_error{"Invalid state"};

    auto const id = unity_screen_service->forward_keep_display_on(sender);
    return std::to_string(id);
}

void repowerd::PowerdService::dbus_clearSysState(
        std::string const& sender,
        std::string const& cookie)
{
    try
    {
        auto const id = std::stoi(cookie);
        unity_screen_service->forward_remove_display_on_request(sender, id);
    }
    catch(...)
    {
    }
}

repowerd::BrightnessParams repowerd::PowerdService::dbus_getBrightnessParams()
{
    return brightness_params;
}
