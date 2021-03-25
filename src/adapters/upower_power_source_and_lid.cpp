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

#include "upower_power_source_and_lid.h"
#include "device_config.h"
#include "event_loop_handler_registration.h"
#include "scoped_g_error.h"
#include "temporary_suspend_inhibition.h"

#include "src/core/log.h"
#include <algorithm>

namespace
{
char const* const log_tag = "UPowerPowerSourceAndLid";
auto const null_handler = []{};
auto const null_arg_handler = [](auto){};
char const* const dbus_upower_name = "org.freedesktop.UPower";
char const* const dbus_upower_path = "/org/freedesktop/UPower";
char const* const dbus_upower_interface = "org.freedesktop.UPower";
char const* const display_device_path = "/org/freedesktop/UPower/devices/DisplayDevice";

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
    battery,
    ups
};

std::string device_type_to_str(uint32_t type)
{
    if (type == static_cast<uint32_t>(DeviceType::unknown))
        return "unknown";
    else if (type == static_cast<uint32_t>(DeviceType::line_power))
        return "line_power";
    else if (type == static_cast<uint32_t>(DeviceType::battery))
        return "battery";
    else if (type == static_cast<uint32_t>(DeviceType::ups))
        return "ups";
    else
        return "unsupported(" + std::to_string(type) + ")";
}

std::string device_state_to_str(uint32_t state)
{
    if (state == static_cast<uint32_t>(DeviceState::unknown))
        return "unknown";
    else if (state == static_cast<uint32_t>(DeviceState::charging))
        return "charging";
    else if (state == static_cast<uint32_t>(DeviceState::discharging))
        return "discharging";
    else if (state == static_cast<uint32_t>(DeviceState::empty))
        return "empty";
    else if (state == static_cast<uint32_t>(DeviceState::fully_charged))
        return "fully_charged";
    else if (state == static_cast<uint32_t>(DeviceState::pending_charge))
        return "pending_charge";
    else if (state == static_cast<uint32_t>(DeviceState::pending_discharge))
        return "pending_discharge";
    else
        return "unsupported(" + std::to_string(state) + ")";
}

double get_critical_temperature(repowerd::DeviceConfig const& device_config)
try
{
    auto const ct_str = device_config.get("shutdownBatteryTemperature", "680");
    return ((double)std::stoi(ct_str)) * 0.1;
}
catch (...)
{
    return 68.0;
}

double constexpr critical_percentage{2.0};
double constexpr non_critical_percentage{3.0};

}

repowerd::UPowerPowerSourceAndLid::UPowerPowerSourceAndLid(
    std::shared_ptr<Log> const& log,
    std::shared_ptr<TemporarySuspendInhibition> const& temporary_suspend_inhibition,
    DeviceConfig const& device_config,
    std::string const& dbus_bus_address)
    : log{log},
      temporary_suspend_inhibition{temporary_suspend_inhibition},
      critical_temperature{get_critical_temperature(device_config)},
      dbus_connection{dbus_bus_address},
      dbus_event_loop{"UPower"},
      power_source_change_handler{null_handler},
      power_source_critical_handler{null_handler},
      lid_handler{null_arg_handler},
      started{false},
      highest_seen_percentage{0.0},
      display_device{}
{
}

void repowerd::UPowerPowerSourceAndLid::start_processing()
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

    dbus_event_loop.enqueue(
        [this]
        {
            add_display_device();
            add_existing_batteries();
        }).get();

    started = true;
}

repowerd::HandlerRegistration repowerd::UPowerPowerSourceAndLid::register_power_source_change_handler(
    PowerSourceChangeHandler const& handler)
{
    return EventLoopHandlerRegistration{
        dbus_event_loop,
            [this, &handler] { this->power_source_change_handler = handler; },
            [this] { this->power_source_change_handler = null_handler; }};
}

repowerd::HandlerRegistration repowerd::UPowerPowerSourceAndLid::register_power_source_critical_handler(
    PowerSourceCriticalHandler const& handler)
{
    return EventLoopHandlerRegistration{
        dbus_event_loop,
            [this, &handler] { this->power_source_critical_handler = handler; },
            [this] { this->power_source_critical_handler = null_handler; }};
}

repowerd::HandlerRegistration repowerd::UPowerPowerSourceAndLid::register_lid_handler(
    LidHandler const& handler)
{
    return EventLoopHandlerRegistration{
        dbus_event_loop,
            [this, &handler] { this->lid_handler = handler; },
            [this] { this->lid_handler = null_arg_handler; }};
}

bool repowerd::UPowerPowerSourceAndLid::is_using_battery_power()
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

std::unordered_set<std::string> repowerd::UPowerPowerSourceAndLid::tracked_batteries()
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

void repowerd::UPowerPowerSourceAndLid::handle_dbus_signal(
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

void repowerd::UPowerPowerSourceAndLid::add_display_device()
{
    display_device = create_device(display_device_path);

    highest_seen_percentage = display_device.percentage;

    log_device("add_display_device", display_device);
}

void repowerd::UPowerPowerSourceAndLid::add_existing_batteries()
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

void repowerd::UPowerPowerSourceAndLid::add_device_if_battery(
    std::string const& device_path)
{
    auto const device = create_device(device_path);

    if (device.type == static_cast<uint32_t>(DeviceType::battery))
    {
        log_device("add_device_if_battery", device);
        batteries[device.path] = device;
    }
}

void repowerd::UPowerPowerSourceAndLid::remove_device(std::string const& device_path)
{
    if (batteries.find(device_path) == batteries.end())
        return;

    log->log(log_tag, "remove_device(%s)", device_path.c_str());

    batteries.erase(device_path);
}

void repowerd::UPowerPowerSourceAndLid::change_device(
    std::string const& device_path, GVariantIter* properties_iter)
{
    bool const is_display_device = device_path == display_device_path;

    if (!is_display_device && batteries.find(device_path) == batteries.end())
        return;

    auto& device = is_display_device ? display_device : batteries[device_path];
    auto const old_info = device;
    update_device(device, properties_iter);
    auto new_info = device;

    log_device("change_device", new_info);

    bool critical{false};
    bool change{false};

    if (is_display_device)
    {
        if (old_info.is_present != new_info.is_present ||
            old_info.type != new_info.type)
        {
            change = true;
        }

        if (new_info.is_present && old_info.state != new_info.state)
        {
            if (new_info.state == static_cast<uint32_t>(DeviceState::discharging) ||
                (old_info.state == static_cast<uint32_t>(DeviceState::discharging) &&
                 (new_info.state == static_cast<uint32_t>(DeviceState::charging) ||
                  new_info.state == static_cast<uint32_t>(DeviceState::fully_charged) ||
                  new_info.state == static_cast<uint32_t>(DeviceState::pending_charge))))
            {
                change = true;
            }
        }

        if (new_info.is_present && old_info.percentage != new_info.percentage)
        {
            highest_seen_percentage = std::max(highest_seen_percentage, new_info.percentage);

            if (new_info.percentage <= critical_percentage && is_using_battery_power() &&
                highest_seen_percentage >= non_critical_percentage)
            {
                log->log(log_tag, "Battery energy percentage is at critical level %.1f%%\n",
                         new_info.percentage);
                critical = true;
            }
        }
    }
    else
    {
        if (new_info.is_present && old_info.temperature != new_info.temperature)
        {
            if (new_info.temperature >= critical_temperature)
            {
                log->log(log_tag, "Battery temperature is at critical level %.1f (limit is %.1f)\n",
                         new_info.temperature, critical_temperature);
                critical = true;
            }
        }
    }

    if (critical || change)
    {
        temporary_suspend_inhibition->inhibit_suspend_for(
            std::chrono::seconds{2}, "UPowerPowerSource");
    }

    if (critical)
    {
        highest_seen_percentage = 0.0;
        power_source_critical_handler();
    }

    if (change)
        power_source_change_handler();
}

void repowerd::UPowerPowerSourceAndLid::change_upower(
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
        else if (key_str == "OnBattery")
        {
            log->log(log_tag, "change_upower(), on_battery");
            power_source_change_handler();
        }

        g_variant_unref(value);
    }
}

GVariant* repowerd::UPowerPowerSourceAndLid::get_device_properties(std::string const& device)
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


repowerd::UPowerPowerSourceAndLid::Device
repowerd::UPowerPowerSourceAndLid::create_device(std::string const& device_path)
{
    auto properties = get_device_properties(device_path);

    if (!properties)
        return Device{};

    Device device{};
    device.path = device_path;

    GVariantIter* properties_iter;
    g_variant_get(properties, "(a{sv})", &properties_iter);

    update_device(device, properties_iter);

    g_variant_iter_free(properties_iter);
    g_variant_unref(properties);

    return device;
}


void repowerd::UPowerPowerSourceAndLid::update_device(
    Device& device, GVariantIter* properties_iter)
{
    char const* key_cstr{""};
    GVariant* value{nullptr};

    while (g_variant_iter_next(properties_iter, "{&sv}", &key_cstr, &value))
    {
        auto const key_str = std::string{key_cstr};
        if (key_str == "Type")
            device.type = g_variant_get_uint32(value);
        else if (key_str == "IsPresent")
            device.is_present = g_variant_get_boolean(value);
        else if (key_str == "State")
            device.state = g_variant_get_uint32(value);
        else if (key_str == "Percentage")
            device.percentage = g_variant_get_double(value);
        else if (key_str == "Temperature")
            device.temperature = g_variant_get_double(value);

        g_variant_unref(value);
    }
}

void repowerd::UPowerPowerSourceAndLid::log_device(
    std::string const& method, Device const& device)
{
    auto const msg =
        method + "(%s), type=%s, is_present=%d, state=%s, percentage=%.2f, temperature=%.2f";

    log->log(log_tag, msg.c_str(),
             device.path.c_str(),
             device_type_to_str(device.type).c_str(),
             device.is_present,
             device_state_to_str(device.state).c_str(),
             device.percentage,
             device.temperature);
}
