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

#include "fake_upower.h"

namespace rt = repowerd::test;
using namespace std::literals::string_literals;

namespace
{

char const* const upower_introspection = R"(<!DOCTYPE node PUBLIC '-//freedesktop//DTD D-BUS Object Introspection 1.0//EN' 'http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd'>
<node>
    <interface name='org.freedesktop.UPower'>
        <method name='EnumerateDevices'>
            <arg name='devices' type='ao' direction='out'/>
        </method>
        <signal name='DeviceAdded'>
            <arg name='path' type='o'/>
        </signal>
        <signal name='DeviceRemoved'>
            <arg name='path' type='o'/>
        </signal>
        <property type="b" name="OnBattery" access="read"/>
    </interface>
</node>)";

char const* const upower_device_introspection = R"(<!DOCTYPE node PUBLIC '-//freedesktop//DTD D-BUS Object Introspection 1.0//EN' 'http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd'>
<node>
    <interface name='org.freedesktop.UPower.Device'>
        <property type="u" name="Type" access="read"/>
        <property type="b" name="Online" access="read"/>
        <property type="d" name="Percentage" access="read"/>
        <property type="d" name="Temperature" access="read"/>
        <property type="b" name="IsPresent" access="read"/>
        <property type="u" name="State" access="read"/>
    </interface>
</node>)";

}

rt::FakeUPower::DeviceInfo rt::FakeUPower::DeviceInfo::for_battery(DeviceState state)
{
    DeviceInfo info;
    info.type = DeviceType::battery;
    info.online = false;
    info.percentage = 100.0;
    info.temperature = 18.0;
    info.is_present = true;
    info.state = state;
    return info;
}

rt::FakeUPower::DeviceInfo rt::FakeUPower::DeviceInfo::for_plugged_line_power()
{
    DeviceInfo info;
    info.type = DeviceType::line_power;
    info.online = true;
    info.percentage = 0.0;
    info.temperature = 0.0;
    info.is_present = true;
    info.state = DeviceState::unknown;
    return info;
}

rt::FakeUPower::DeviceInfo rt::FakeUPower::DeviceInfo::for_unplugged_line_power()
{
    DeviceInfo info;
    info.type = DeviceType::line_power;
    info.online = false;
    info.percentage = 0.0;
    info.temperature = 0.0;
    info.is_present = true;
    info.state = DeviceState::unknown;
    return info;
}

rt::FakeUPower::FakeUPower(
    std::string const& dbus_address)
    : rt::DBusClient{dbus_address, "org.freedesktop.UPower", "/org/freedesktop/UPower"},
      num_enumerate_device_calls_{0}
{
    connection.request_name("org.freedesktop.UPower");

    upower_handler_registration = event_loop.register_object_handler(
        connection,
        "/org/freedesktop/UPower",
        upower_introspection,
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

void rt::FakeUPower::add_device(std::string const& device_path, DeviceInfo const& info)
{
    {
        std::lock_guard<std::mutex> lock{devices_mutex};

        devices[device_path] = info;
    }

    device_handler_registrations[device_path] = event_loop.register_object_handler(
        connection,
        device_path.c_str(),
        upower_device_introspection,
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

    auto const params = g_variant_new_parsed("(@o %o,)", device_path.c_str());
    emit_signal_full("/org/freedesktop/UPower", "org.freedesktop.UPower", "DeviceAdded", params);
}

void rt::FakeUPower::change_device(std::string const& device_path, DeviceInfo const& info)
{
    DeviceInfo old_info;

    {
        std::lock_guard<std::mutex> lock{devices_mutex};

        old_info = devices[device_path];
        devices[device_path] = info;
    }

    std::string changed_properties_str;
    if (old_info.type != info.type)
    {
        if (!changed_properties_str.empty()) changed_properties_str += ", ";
        changed_properties_str += "'Type': <@u " + std::to_string(static_cast<int>(info.type)) + ">";
    }
    if (old_info.online != info.online)
    {
        if (!changed_properties_str.empty()) changed_properties_str += ", ";
        changed_properties_str += "'Online': <"s + (info.online ? "true" : "false") + ">";
    }
    if (old_info.percentage != info.percentage)
    {
        if (!changed_properties_str.empty()) changed_properties_str += ", ";
        changed_properties_str += "'Percentage': <" + std::to_string(info.percentage) + ">";
    }
    if (old_info.temperature != info.temperature)
    {
        if (!changed_properties_str.empty()) changed_properties_str += ", ";
        changed_properties_str += "'Temperature': <" + std::to_string(info.temperature) + ">";
    }
    if (old_info.is_present != info.is_present)
    {
        if (!changed_properties_str.empty()) changed_properties_str += ", ";
        changed_properties_str += "'IsPresent': <"s + (info.is_present ? "true" : "false") + ">";
    }
    if (old_info.state != info.state)
    {
        if (!changed_properties_str.empty()) changed_properties_str += ", ";
        changed_properties_str += "'State': <@u " + std::to_string(static_cast<int>(info.state)) + ">";
    }

    auto const params_str =
        "(@s 'org.freedesktop.UPower.Device',"s +
        " @a{sv} {" + changed_properties_str + "}," +
        " @as [])";

    auto const params = g_variant_new_parsed(params_str.c_str());
    emit_signal_full(device_path.c_str(), "org.freedesktop.DBus.Properties", "PropertiesChanged", params);
}

void rt::FakeUPower::remove_device(std::string const& device_path)
{
    {
        std::lock_guard<std::mutex> lock{devices_mutex};
        devices.erase(device_path);
    }

    device_handler_registrations.erase(device_path);

    auto const params = g_variant_new_parsed("(@o %o,)", device_path.c_str());
    emit_signal_full("/org/freedesktop/UPower", "org.freedesktop.UPower", "DeviceRemoved", params);
}

void rt::FakeUPower::close_lid()
{
    auto const params_str =
        "(@s 'org.freedesktop.UPower',"s +
        " @a{sv} {'LidIsClosed': <@b true>}," +
        " @as [])";
    auto const params = g_variant_new_parsed(params_str.c_str());

    emit_signal_full(
        "/org/freedesktop/UPower",
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        params);
}

void rt::FakeUPower::open_lid()
{
    auto const params_str =
        "(@s 'org.freedesktop.UPower',"s +
        " @a{sv} {'LidIsClosed': <@b false>}," +
        " @as [])";
    auto const params = g_variant_new_parsed(params_str.c_str());

    emit_signal_full(
        "/org/freedesktop/UPower",
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        params);
}

int rt::FakeUPower::num_enumerate_devices_calls()
{
    return num_enumerate_device_calls_;
}

void rt::FakeUPower::dbus_method_call(
    GDBusConnection* /*connection*/,
    gchar const* /*sender_cstr*/,
    gchar const* object_path_cstr,
    gchar const* interface_name_cstr,
    gchar const* method_name_cstr,
    GVariant* /*parameters*/,
    GDBusMethodInvocation* invocation)
{
    std::string const object_path{object_path_cstr ? object_path_cstr : ""};
    std::string const method_name{method_name_cstr ? method_name_cstr : ""};
    std::string const interface_name{interface_name_cstr ? interface_name_cstr : ""};

    if (method_name == "EnumerateDevices")
    {
        ++num_enumerate_device_calls_;
        std::string devices_str = "([";
        int count = 0;
        for (auto const& device : devices)
        {
            if (count++ > 0) devices_str += ", ";
            devices_str += "@o '" + device.first + "'";
        }
        devices_str += "],)";

        auto const devices = g_variant_new_parsed(devices_str.c_str());
        g_dbus_method_invocation_return_value(invocation, devices);
    }
    else if (interface_name == "org.freedesktop.DBus.Properties" &&
             method_name == "GetAll")
    {
        DeviceInfo info;
        {
            std::lock_guard<std::mutex> lock{devices_mutex};
            info = devices[object_path];
        }

        auto const properties = g_variant_new_parsed(
            "(@a{sv} {'Type': <%u>, 'Online': <%b>, 'Percentage': <%d>, "
            "'Temperature': <%d>, 'IsPresent': <%b>, 'State': <%u>},)",
            info.type, info.online, info.percentage,
            info.temperature, info.is_present, info.state);

        g_dbus_method_invocation_return_value(invocation, properties);
    }
    else if (interface_name == "org.freedesktop.DBus.Properties" &&
             method_name == "Get" && object_path == "/org/freedesktop/UPower")
    {
        auto const properties = g_variant_new_parsed("(<%b>,)", is_using_battery_power());

        g_dbus_method_invocation_return_value(invocation, properties);
    }
    else
    {
        g_dbus_method_invocation_return_error_literal(
            invocation, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED, "");
    }
}

bool rt::FakeUPower::is_using_battery_power()
{
    bool on_battery = false;

    for (auto const& device : devices)
    {
        auto const& info = device.second;
        if (info.type == DeviceType::battery &&
            info.state == DeviceState::discharging &&
            info.is_present)
        {
            on_battery = true;
        }
    }

    for (auto const& device : devices)
    {
        auto const& info = device.second;
        if (info.type == DeviceType::line_power &&
            info.online)
        {
            on_battery = false;
        }
    }

    return on_battery;
}
