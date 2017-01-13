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

#include "unity_display_power_control.h"
#include "scoped_g_error.h"

#include <gio/gio.h>

namespace
{
char const* const unity_display_bus_name = "com.canonical.Unity.Display";
char const* const unity_display_object_path = "/com/canonical/Unity/Display";
char const* const unity_display_interface_name = "com.canonical.Unity.Display";
char const* const log_tag = "UnityDisplayPowerControl";
}

repowerd::UnityDisplayPowerControl::UnityDisplayPowerControl(
    std::shared_ptr<Log> const& log,
    std::string const& dbus_bus_address)
    : log{log},
      dbus_connection{dbus_bus_address},
      dbus_event_loop{"Display"},
      has_active_external_displays_{false}
{
    dbus_signal_handler_registration = dbus_event_loop.register_signal_handler(
        dbus_connection,
        unity_display_bus_name,
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        unity_display_object_path,
        [this] (
            GDBusConnection* connection,
            gchar const* sender,
            gchar const* object_path,
            gchar const* interface_name,
            gchar const* signal_name,
            GVariant* parameters)
        {
            handle_dbus_signal(
                connection, sender, object_path, interface_name,
                signal_name, parameters);
        });

    dbus_event_loop.enqueue([this] { dbus_query_active_outputs(); }).get();
}

void repowerd::UnityDisplayPowerControl::turn_on()
{
    log->log(log_tag, "turn_on()");

    g_dbus_connection_call(
        dbus_connection,
        unity_display_bus_name,
        unity_display_object_path,
        unity_display_interface_name,
        "TurnOn",
        nullptr,
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        nullptr,
        nullptr);
}

void repowerd::UnityDisplayPowerControl::turn_off()
{
    log->log(log_tag, "turn_off()");

    g_dbus_connection_call(
        dbus_connection,
        unity_display_bus_name,
        unity_display_object_path,
        unity_display_interface_name,
        "TurnOff",
        nullptr,
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        nullptr,
        nullptr);
}

bool repowerd::UnityDisplayPowerControl::has_active_external_displays()
{
    return has_active_external_displays_;
}

void repowerd::UnityDisplayPowerControl::handle_dbus_signal(
    GDBusConnection* /*connection*/,
    gchar const* /*sender*/,
    gchar const* /*object_path*/,
    gchar const* /*interface_name*/,
    gchar const* signal_name_cstr,
    GVariant* parameters)
{
    std::string const signal_name{signal_name_cstr ? signal_name_cstr : ""};

    if (signal_name == "PropertiesChanged")
    {
        char const* properties_interface_cstr{""};
        GVariantIter* properties_iter{nullptr};
        g_variant_get(parameters, "(&sa{sv}as)",
                      &properties_interface_cstr, &properties_iter, nullptr);

        std::string const properties_interface{properties_interface_cstr};

        if (properties_interface == unity_display_interface_name)
            dbus_PropertiesChanged(properties_iter);

        g_variant_iter_free(properties_iter);
    }
}

void repowerd::UnityDisplayPowerControl::dbus_PropertiesChanged(
    GVariantIter* properties_iter)
{
    char const* key_cstr{""};
    GVariant* value{nullptr};

    while (g_variant_iter_next(properties_iter, "{&sv}", &key_cstr, &value))
    {
        auto const key_str = std::string{key_cstr ? key_cstr : ""};

        if (key_str == "ActiveOutputs")
        {
            gint32 internal{0};
            gint32 external{0};

            g_variant_get(value, "(ii)", &internal, &external);

            dbus_ActiveOutputs(internal, external);
        }

        g_variant_unref(value);
    }
}

void repowerd::UnityDisplayPowerControl::dbus_ActiveOutputs(
    int32_t internal, int32_t external)
{
    log->log(log_tag, "dbus_ActiveOutputs(internal=%d, external=%d)", internal, external);

    has_active_external_displays_ = (external > 0);
}

void repowerd::UnityDisplayPowerControl::dbus_query_active_outputs()
{
    log->log(log_tag, "dbus_query_active_outputs()");

    int constexpr timeout_default = -1;
    auto constexpr null_cancellable = nullptr;
    ScopedGError error;

    auto const result = g_dbus_connection_call_sync(
        dbus_connection,
        unity_display_bus_name,
        unity_display_object_path,
        "org.freedesktop.DBus.Properties",
        "Get",
        g_variant_new("(ss)", unity_display_interface_name, "ActiveOutputs"),
        G_VARIANT_TYPE("(v)"),
        G_DBUS_CALL_FLAGS_NONE,
        timeout_default,
        null_cancellable,
        error);

    if (!result)
    {
        log->log(log_tag, "dbus_get_active_outputs() failed to get ActiveOutputs: %s",
                 error.message_str().c_str());
        return;
    }

    GVariant* property_variant{nullptr};
    g_variant_get(result, "(v)", &property_variant);

    gint32 internal{0};
    gint32 external{0};

    g_variant_get(property_variant, "(ii)", &internal, &external);

    dbus_ActiveOutputs(internal, external);

    g_variant_unref(property_variant);
    g_variant_unref(result);
}
