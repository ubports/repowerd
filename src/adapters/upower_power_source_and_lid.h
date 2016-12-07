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

#include "src/core/power_source.h"
#include "src/core/lid.h"

#include "dbus_connection_handle.h"
#include "dbus_event_loop.h"

#include <unordered_map>
#include <unordered_set>

namespace repowerd
{
class Log;
class DeviceConfig;
class TemporarySuspendInhibition;

class UPowerPowerSourceAndLid : public PowerSource, public Lid
{
public:
    UPowerPowerSourceAndLid(
        std::shared_ptr<Log> const& log,
        std::shared_ptr<TemporarySuspendInhibition> const& temporary_suspend_inhibition,
        DeviceConfig const& device_config,
        std::string const& dbus_bus_address);

    void start_processing() override;

    HandlerRegistration register_power_source_change_handler(
        PowerSourceChangeHandler const& handler) override;

    HandlerRegistration register_power_source_critical_handler(
        PowerSourceCriticalHandler const& handler) override;

    HandlerRegistration register_lid_handler(
        LidHandler const& handler) override;

    std::unordered_set<std::string> tracked_batteries();

private:
    void handle_dbus_signal(
        GDBusConnection* connection,
        gchar const* sender,
        gchar const* object_path,
        gchar const* interface_name,
        gchar const* signal_name,
        GVariant* parameters);

    void add_existing_batteries();
    void add_device_if_battery(std::string const& device);
    void remove_device(std::string const& device);
    void change_device(std::string const& device, GVariantIter* properties_iter);
    GVariant* get_device_properties(std::string const& device);
    bool is_using_battery_power();
    void change_upower(GVariantIter* properties_iter);

    struct BatteryInfo
    {
        bool is_present;
        uint32_t state;
        double percentage;
        double temperature;
    };

    std::shared_ptr<Log> const log;
    std::shared_ptr<TemporarySuspendInhibition> const temporary_suspend_inhibition;
    double const critical_temperature;

    DBusConnectionHandle dbus_connection;
    DBusEventLoop dbus_event_loop;
    HandlerRegistration dbus_signal_handler_registration;

    PowerSourceChangeHandler power_source_change_handler;
    PowerSourceChangeHandler power_source_critical_handler;
    LidHandler lid_handler;

    bool started;

    std::unordered_map<std::string,BatteryInfo> batteries;
};

}
