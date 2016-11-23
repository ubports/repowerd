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

#include "upower_power_source.h"
#include "device_config.h"
#include "event_loop_handler_registration.h"
#include "scoped_g_error.h"

#include "src/core/log.h"

namespace
{
char const* const log_tag = "UPowerPowerSource";
auto const null_handler = []{};
auto const null_arg_handler = [](auto){};
char const* const dbus_upower_name = "org.freedesktop.UPower";
char const* const dbus_upower_path = "/org/freedesktop/UPower";
char const* const dbus_upower_interface = "org.freedesktop.UPower";

enum class DeviceState
{ 
    unknown = 0,
    charging,
    discharging,
    empty,
    fully_charged,
    pending_charge,
    pending_discharge
};

enum class DeviceType
{ 
    unknown = 0,
    line_power,
    battery
};

double get_critical_temperature(repowerd::DeviceConfig const& device_config)
try
{
    auto const ct_str = device_config.get("shutdownBatteryTemperature", "680");
    return std::stod(ct_str) * 0.1;
}
catch (...)
{
    return 68.0;
}

}

repowerd::UPowerPowerSource::UPowerPowerSource(
    std::shared_ptr<Log> const& log,
    DeviceConfig const& device_config,
    std::string const& dbus_bus_address)
    : log{log},
      critical_temperature{get_critical_temperature(device_config)},
      dbus_connection{dbus_bus_address},
      power_source_change_handler{null_handler},
      power_source_critical_handler{null_handler},
      lid_handler{null_arg_handler},
      started{false}
{
}

void repowerd::UPowerPowerSource::start_processing()
{
    if (started) return;

    dbus_signal_handler_registration = dbus_event_loop.register_signal_handler(
        dbus_connection,
        dbus_upower_name,
        nullptr,
        nullptr,
        nullptr,
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

    dbus_event_loop.enqueue([this] { add_existing_batteries(); }).get();

    started = true;
}

repowerd::HandlerRegistration repowerd::UPowerPowerSource::register_power_source_change_handler(
    PowerSourceChangeHandler const& handler)
{
    return EventLoopHandlerRegistration{
        dbus_event_loop,
            [this, &handler] { this->power_source_change_handler = handler; },
            [this] { this->power_source_change_handler = null_handler; }};
}

repowerd::HandlerRegistration repowerd::UPowerPowerSource::register_power_source_critical_handler(
    PowerSourceCriticalHandler const& handler)
{
    return EventLoopHandlerRegistration{
        dbus_event_loop,
            [this, &handler] { this->power_source_critical_handler = handler; },
            [this] { this->power_source_critical_handler = null_handler; }};
}

repowerd::HandlerRegistration repowerd::UPowerPowerSource::register_lid_handler(
    LidHandler const& handler)
{
    return EventLoopHandlerRegistration{
        dbus_event_loop,
            [this, &handler] { this->lid_handler = handler; },
            [this] { this->lid_handler = null_arg_handler; }};
}

std::unordered_set<std::string> repowerd::UPowerPowerSource::tracked_batteries()
{
    std::unordered_set<std::string> ret_batteries;
    dbus_event_loop.enqueue(
        [this, &ret_batteries]
        {
            for (auto const& battery : batteries)
                ret_batteries.insert(battery.first);
        }).get();
    return ret_batteries;
}

void repowerd::UPowerPowerSource::handle_dbus_signal(
    GDBusConnection* /*connection*/,
    gchar const* /*sender*/,
    gchar const* object_path_cstr,
    gchar const* /*interface_name*/,
    gchar const* signal_name_cstr,
    GVariant* parameters)
{
    std::string const object_path{object_path_cstr ? object_path_cstr : ""};
    std::string const signal_name{signal_name_cstr ? signal_name_cstr : ""};

    if (signal_name == "PropertiesChanged")
    {
        char const* properties_interface_cstr{""};
        GVariantIter* properties_iter;
        g_variant_get(parameters, "(&sa{sv}as)",
                      &properties_interface_cstr, &properties_iter, nullptr);

        std::string const properties_interface{properties_interface_cstr};

        if (properties_interface == "org.freedesktop.UPower.Device")
            change_device(object_path, properties_iter);
        else if (properties_interface == "org.freedesktop.UPower")
            change_upower(properties_iter);

        g_variant_iter_free(properties_iter);
    }
    else if (signal_name == "DeviceAdded")
    {
        char const* device{""};
        g_variant_get(parameters, "(&o)", &device);

        add_device_if_battery(device);
    }
    else if (signal_name == "DeviceRemoved")
    {
        char const* device{""};
        g_variant_get(parameters, "(&o)", &device);

        remove_device(device);
    }
}

void repowerd::UPowerPowerSource::add_existing_batteries()
{
    int constexpr timeout_default = -1;
    auto constexpr null_cancellable = nullptr;
    auto constexpr null_args = nullptr;
    ScopedGError error;

    auto const result = g_dbus_connection_call_sync(
        dbus_connection,
        dbus_upower_name,
        dbus_upower_path,
        dbus_upower_interface,
        "EnumerateDevices",
        null_args,
        G_VARIANT_TYPE("(ao)"),
        G_DBUS_CALL_FLAGS_NONE,
        timeout_default,
        null_cancellable,
        error);

    if (!result)
    {
        log->log(log_tag, "add_existing_batteries() failed to EnumerateDevices: %s",
                 error.message_str().c_str());
        return;
    }

    GVariantIter* result_devices;
    g_variant_get(result, "(ao)", &result_devices);

    char const* device{""};
    while (g_variant_iter_next(result_devices, "&o", &device))
        add_device_if_battery(device);

    g_variant_iter_free(result_devices);
    g_variant_unref(result);
}

void repowerd::UPowerPowerSource::add_device_if_battery(std::string const& device)
{
    auto properties = get_device_properties(device);

    if (!properties)
        return;

    char const* key_cstr{""};
    GVariant* value{nullptr};
    BatteryInfo battery_info;
    uint32_t device_type{0};

    GVariantIter* properties_iter;
    g_variant_get(properties, "(a{sv})", &properties_iter);

    while (g_variant_iter_next(properties_iter, "{&sv}", &key_cstr, &value))
    {
        auto const key_str = std::string{key_cstr};
        if (key_str == "Type")
            device_type = g_variant_get_uint32(value);
        else if (key_str == "IsPresent")
            battery_info.is_present = g_variant_get_boolean(value);
        else if (key_str == "State")
            battery_info.state = g_variant_get_uint32(value);
        else if (key_str == "Percentage")
            battery_info.percentage = g_variant_get_double(value);
        else if (key_str == "Temperature")
            battery_info.temperature = g_variant_get_double(value);

        g_variant_unref(value);
    }

    g_variant_iter_free(properties_iter);
    g_variant_unref(properties);

    if (device_type == static_cast<uint32_t>(DeviceType::battery))
    {
        log->log(log_tag, "add_device_if_battery(%s), "
                 "is_present=%d, state=%d, percentage=%.2f, temperature=%.2f",
                 device.c_str(),
                 battery_info.is_present,
                 battery_info.state,
                 battery_info.percentage,
                 battery_info.temperature);

        batteries[device] = battery_info;
    }
}

void repowerd::UPowerPowerSource::remove_device(std::string const& device)
{
    if (batteries.find(device) == batteries.end())
        return;

    log->log(log_tag, "remove_device(%s)", device.c_str());

    batteries.erase(device);
}

void repowerd::UPowerPowerSource::change_device(
    std::string const& device, GVariantIter* properties_iter)
{
    if (batteries.find(device) == batteries.end())
        return;

    auto const old_info = batteries[device];

    char const* key_cstr{""};
    GVariant* value{nullptr};
    auto new_info = old_info;

    while (g_variant_iter_next(properties_iter, "{&sv}", &key_cstr, &value))
    {
        auto const key_str = std::string{key_cstr};

        if (key_str == "State")
            new_info.state = g_variant_get_uint32(value);
        else if (key_str == "Percentage")
            new_info.percentage = g_variant_get_double(value);
        else if (key_str == "Temperature")
            new_info.temperature = g_variant_get_double(value);
        else if (key_str == "IsPresent")
            new_info.is_present = g_variant_get_boolean(value);

        g_variant_unref(value);
    }

    log->log(log_tag, "change_device(%s), "
             "is_present=%d, state=%d, percentage=%.2f, temperature=%.2f",
             device.c_str(),
             new_info.is_present,
             new_info.state,
             new_info.percentage,
             new_info.temperature);

    batteries[device] = new_info;

    bool critical{false};
    bool change{false};

    if (old_info.is_present != new_info.is_present)
    {
        change = true;
    }

    if (new_info.is_present && old_info.state != new_info.state)
    {
        if (new_info.state == static_cast<uint32_t>(DeviceState::discharging) ||
            (old_info.state == static_cast<uint32_t>(DeviceState::discharging) &&
             (new_info.state == static_cast<uint32_t>(DeviceState::charging) ||
              new_info.state == static_cast<uint32_t>(DeviceState::fully_charged))))
        {
            change = true;
        }
    }

    if (new_info.is_present && old_info.percentage != new_info.percentage)
    {
        if (new_info.percentage <= 1.0 && is_using_battery_power())
        {
            log->log(log_tag, "Battery energy percentage is at critical level %.1f%%\n",
                     new_info.percentage);
            critical = true;
        }
    }

    if (new_info.is_present && old_info.temperature != new_info.temperature)
    {
        if (new_info.temperature >= critical_temperature)
        {
            log->log(log_tag, "Battery temperature is at critical level %.1f (limit is %.1f)\n",
                     new_info.temperature, critical_temperature);
            critical = true;
        }
    }

    if (critical)
        power_source_critical_handler();

    if (change)
        power_source_change_handler();
}

GVariant* repowerd::UPowerPowerSource::get_device_properties(std::string const& device)
{
    int constexpr timeout_default = -1;
    auto constexpr null_cancellable = nullptr;
    ScopedGError error;

    auto const result = g_dbus_connection_call_sync(
        dbus_connection,
        dbus_upower_name,
        device.c_str(),
        "org.freedesktop.DBus.Properties",
        "GetAll",
        g_variant_new("(s)", "org.freedesktop.UPower.Device"),
        G_VARIANT_TYPE("(a{sv})"),
        G_DBUS_CALL_FLAGS_NONE,
        timeout_default,
        null_cancellable,
        error);

    if (!result)
    {
        log->log(log_tag, "get_device_properties() failed: %s",
                 error.message_str().c_str());
    }

    return result;
}

bool repowerd::UPowerPowerSource::is_using_battery_power()
{
    int constexpr timeout = 1000;
    auto constexpr null_cancellable = nullptr;
    ScopedGError error;

    auto const result = g_dbus_connection_call_sync(
        dbus_connection,
        dbus_upower_name,
        "/org/freedesktop/UPower",
        "org.freedesktop.DBus.Properties",
        "Get",
        g_variant_new("(ss)", "org.freedesktop.UPower", "OnBattery"),
        G_VARIANT_TYPE("(v)"),
        G_DBUS_CALL_FLAGS_NONE,
        timeout,
        null_cancellable,
        error);

    if (!result)
    {
        log->log(log_tag, "is_using_battery_power() failed, assuming true, more info: %s",
                 error.message_str().c_str());
        return true;
    }

    GVariant* on_battery;
    g_variant_get(result, "(v)", &on_battery);

    auto const ret = g_variant_get_boolean(on_battery);

    g_variant_unref(on_battery);
    g_variant_unref(result);

    log->log(log_tag, "is_using_battery_power() => %s", ret ? "true" : "false");

    return ret;
}

void repowerd::UPowerPowerSource::change_upower(
    GVariantIter* properties_iter)
{
    char const* key_cstr{""};
    GVariant* value{nullptr};

    while (g_variant_iter_next(properties_iter, "{&sv}", &key_cstr, &value))
    {
        auto const key_str = std::string{key_cstr};

        if (key_str == "LidIsClosed")
        {
            auto const lid_is_closed = g_variant_get_boolean(value);
            log->log(log_tag, "change_upower(), lid_is_closed=%s", 
                     lid_is_closed ? "true" : "false");
            if (lid_is_closed)
                lid_handler(LidState::closed);
            else
                lid_handler(LidState::open);
        }

        g_variant_unref(value);
    }
}
