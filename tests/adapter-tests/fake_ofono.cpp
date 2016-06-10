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

#include "fake_ofono.h"

namespace rt = repowerd::test;

namespace
{

char const* const ofono_manager_introspection = R"(<!DOCTYPE node PUBLIC '-//freedesktop//DTD D-BUS Object Introspection 1.0//EN' 'http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd'>
<node>
    <interface name='org.ofono.Manager'>
        <method name='GetModems'>
            <arg name='modems' type='a(oa{sv})' direction='out'/>
        </method>
        <signal name='ModemAdded'>
            <arg name='path' type='o'/>
            <arg name='properties' type='a{sv}'/>
        </signal>
        <signal name='ModemRemoved'>
            <arg name='path' type='o'/>
        </signal>
    </interface>
</node>)";

char const* const ofono_modem_introspection = R"(<!DOCTYPE node PUBLIC '-//freedesktop//DTD D-BUS Object Introspection 1.0//EN' 'http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd'>
<node>
    <interface name='org.ofono.RadioSettings'>
        <method name='SetProperty'>
            <arg name='property' type='s' direction='in'/>
            <arg name='value' type='v' direction='in'/>
        </method>
    </interface>
</node>)";

std::string ofono_call_state_to_string(repowerd::OfonoCallState state)
{
    switch (state)
    {
    case repowerd::OfonoCallState::active:
        return "active";
    case repowerd::OfonoCallState::alerting:
        return "alerting";
    case repowerd::OfonoCallState::dialing:
        return "dialing";
    case repowerd::OfonoCallState::disconnected:
        return "disconnected";
    case repowerd::OfonoCallState::held:
        return "held";
    case repowerd::OfonoCallState::incoming:
        return "incoming";
    case repowerd::OfonoCallState::waiting:
        return "waiting";
    case repowerd::OfonoCallState::invalid:
    default:
        return "";
    };

    return "";
}

}

rt::FakeOfono::FakeOfono(
    std::string const& dbus_address)
    : rt::DBusClient{dbus_address, "org.ofono", "/phonesim"}
{
    connection.request_name("org.ofono");

    manager_handler_registration = event_loop.register_object_handler(
        connection,
        "/",
        ofono_manager_introspection,
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

void rt::FakeOfono::add_call(
    std::string const& call_path, repowerd::OfonoCallState call_state)
{
    auto const params = g_variant_new_parsed(
        "(@o %o, @a{sv} {'Name': <'bla'>, 'State': <%s>, 'Emergency': <false>})",
        call_path.c_str(), ofono_call_state_to_string(call_state).c_str());
    emit_signal("org.ofono.VoiceCallManager", "CallAdded", params);
}

void rt::FakeOfono::remove_call(std::string const& call_path)
{
    auto const params = g_variant_new_parsed("(@o %o,)", call_path.c_str());
    emit_signal("org.ofono.VoiceCallManager", "CallRemoved", params);
}

void rt::FakeOfono::change_call_state(
    std::string const& call_path, repowerd::OfonoCallState call_state)
{
    auto const params = g_variant_new_parsed(
        "('State',<%s>)",
        ofono_call_state_to_string(call_state).c_str());

    emit_signal_full(call_path.c_str(), "org.ofono.VoiceCall", "PropertyChanged", params);
}

void rt::FakeOfono::add_modem(std::string const& modem_path)
{
    {
        std::lock_guard<std::mutex> lock{modems_mutex};

        modems[modem_path] = ModemPowerState::normal;
        modems_cv.notify_one();
    }

    modem_handler_registrations[modem_path] = event_loop.register_object_handler(
        connection,
        modem_path.c_str(),
        ofono_modem_introspection,
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

    auto const params = g_variant_new_parsed("(@o %o, @a{sv} {})", modem_path.c_str());
    emit_signal_full("/", "org.ofono.Manager", "ModemAdded", params);
}

void rt::FakeOfono::remove_modem(std::string const& modem_path)
{
    {
        std::lock_guard<std::mutex> lock{modems_mutex};
        modems.erase(modem_path);
        modems_cv.notify_one();
    }

    modem_handler_registrations.erase(modem_path);

    auto const params = g_variant_new_parsed("(@o %o,)", modem_path.c_str());
    emit_signal_full("/", "org.ofono.Manager", "ModemRemoved", params);
}

bool rt::FakeOfono::wait_for_modems_condition(
    std::function<bool(Modems const&)> const& condition,
    std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock{modems_mutex};

    return modems_cv.wait_for(lock, timeout, [this,&condition]{ return condition(modems);});
}

void rt::FakeOfono::dbus_method_call(
    GDBusConnection* /*connection*/,
    gchar const* /*sender_cstr*/,
    gchar const* object_path_cstr,
    gchar const* interface_name_cstr,
    gchar const* method_name_cstr,
    GVariant* parameters,
    GDBusMethodInvocation* invocation)
{
    std::string const object_path{object_path_cstr ? object_path_cstr : ""};
    std::string const method_name{method_name_cstr ? method_name_cstr : ""};
    std::string const interface_name{interface_name_cstr ? interface_name_cstr : ""};

    if (method_name == "GetModems")
    {
        std::string modems_str = "([";
        int count = 0;
        for (auto const& modem : modems)
        {
            if (count++ > 0) modems_str += ", ";
            modems_str += "(@o '" + modem.first + "', @a{sv} {})";
        }
        modems_str += "],)";

        auto const modems = g_variant_new_parsed(modems_str.c_str());
        g_dbus_method_invocation_return_value(invocation, modems);
    }
    else if (interface_name == "org.ofono.RadioSettings" && method_name == "SetProperty")
    {
        char const* property{""};
        GVariant* value{nullptr};
        g_variant_get(parameters, "(&sv)", &property, &value);

        if (std::string{property} == "FastDormancy")
        {
            std::lock_guard<std::mutex> lock{modems_mutex};

            auto const fast_dormancy = g_variant_get_boolean(value);

            if (fast_dormancy)
                modems[object_path] = ModemPowerState::low;
            else
                modems[object_path] = ModemPowerState::normal;

            modems_cv.notify_one();
        }

        g_variant_unref(value);
        g_dbus_method_invocation_return_value(invocation, nullptr);
    }
}
