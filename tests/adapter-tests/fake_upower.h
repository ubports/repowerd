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

#pragma once

#include "dbus_client.h"

#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <chrono>
#include <atomic>

namespace repowerd
{
namespace test
{

class FakeUPower : private DBusClient
{
public:
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

    struct DeviceInfo
    { 
        static DeviceInfo for_battery(DeviceState state);
        static DeviceInfo for_plugged_line_power();
        static DeviceInfo for_unplugged_line_power();
        DeviceType type;
        bool online;
        double percentage;
        double temperature;
        bool is_present;
        DeviceState state;
    };


    FakeUPower(std::string const& dbus_address);

    void add_device(std::string const& device_path, DeviceInfo const& info);
    void remove_device(std::string const& device_path);
    void change_device(std::string const& device_path, DeviceInfo const& info);

    void close_lid();
    void open_lid();

    int num_enumerate_devices_calls();

private:
    void dbus_method_call(
        GDBusConnection* connection,
        gchar const* sender_cstr,
        gchar const* object_path_cstr,
        gchar const* interface_name_cstr,
        gchar const* method_name_cstr,
        GVariant* parameters,
        GDBusMethodInvocation* invocation);
    bool is_using_battery_power();

    repowerd::HandlerRegistration upower_handler_registration;

    std::atomic<int> num_enumerate_device_calls_;

    std::mutex devices_mutex;
    std::unordered_map<std::string,DeviceInfo> devices;
    std::unordered_map<std::string,HandlerRegistration> device_handler_registrations;
};

}
}
