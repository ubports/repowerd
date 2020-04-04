/*
 * Copyright © 2020 UBports foundation
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
 * Authored by: Marius Gripsgard <marius@ubports.com>
 */

#include "sensorfw_common.h"

#include <QObject>

#include "socketreader.h"

namespace
{
char const* const log_tag = "Sensorfw";

auto const null_handler = [](double){};
char const* const dbus_sensorfw_name = "com.nokia.SensorService";
char const* const dbus_sensorfw_path = "/SensorManager";
char const* const dbus_sensorfw_interface = "local.SensorManager";
}

repowerd::Sensorfw::Sensorfw(
    std::shared_ptr<Log> const& log,
    std::string const& dbus_bus_address,
    std::string const& name,
    PluginType const& plugin)
    : log{log},
      dbus_connection{dbus_bus_address},
      dbus_event_loop{name},
      m_socket(std::make_shared<SocketReader>()),
      m_plugin(plugin),
      m_pid(getpid())
{
    if (!load_plugin())
        throw "Could not create sensorfw backend";

    request_sensor();

    log->log(log_tag, "Got plugin_string %s", plugin_string());
    log->log(log_tag, "Got plugin_interface %s", plugin_interface());
    log->log(log_tag, "Got plugin_path %s", plugin_path());

    m_socket->initiateConnection(m_sessionid);
}

repowerd::Sensorfw::~Sensorfw()
{
    stop();
    release_sensor();
    m_socket->dropConnection();
}

const char* repowerd::Sensorfw::plugin_string() const
{
    switch (m_plugin) {
        case PluginType::LIGHT: return "alssensor";
        case PluginType::PROXIMITY: return "proximitysensor";
    }

    return "";
}

const char* repowerd::Sensorfw::plugin_interface() const
{
    switch (m_plugin) {
        case PluginType::LIGHT: return "local.ALSSensor";
        case PluginType::PROXIMITY: return "local.ProximitySensor";
    }

    return "";
}

const char* repowerd::Sensorfw::plugin_path() const
{
    char *new_str;
    if (asprintf(&new_str,"%s/%s", dbus_sensorfw_path, plugin_string()) == -1)
        return "";

    return new_str;
}

bool repowerd::Sensorfw::load_plugin()
{
    int constexpr timeout_default = 100;
    auto const result =  g_dbus_connection_call_sync(
            dbus_connection,
            dbus_sensorfw_name,
            dbus_sensorfw_path,
            dbus_sensorfw_interface,
            "loadPlugin",
            g_variant_new("(s)", plugin_string()),
            G_VARIANT_TYPE("(b)"),
            G_DBUS_CALL_FLAGS_NONE,
            timeout_default,
            NULL,
            NULL);

    if (!result)
    {
        log->log(log_tag, "failed to call load_plugin");
        return false;
    }

    gboolean the_result;
    g_variant_get(result, "(b)", &the_result);
    g_variant_unref(result);

    return the_result;
}

void repowerd::Sensorfw::request_sensor()
{
    int constexpr timeout_default = 100;
    auto const result =  g_dbus_connection_call_sync(
            dbus_connection,
            dbus_sensorfw_name,
            dbus_sensorfw_path,
            dbus_sensorfw_interface,
            "requestSensor",
            g_variant_new("(sx)", plugin_string(), m_pid),
            G_VARIANT_TYPE("(i)"),
            G_DBUS_CALL_FLAGS_NONE,
            timeout_default,
            NULL,
            NULL);

    if (!result)
    {
        log->log(log_tag, "failed to call request_sensor");
        return;
    }

    gint32 the_result;
    g_variant_get(result, "(i)", &the_result);
    g_variant_unref(result);

    m_sessionid = the_result;

    log->log(log_tag, "Got new plugin for %s with pid %i and session %i", plugin_string(), m_pid, m_sessionid);
}

bool repowerd::Sensorfw::release_sensor()
{
    int constexpr timeout_default = 100;
    auto const result =  g_dbus_connection_call_sync(
            dbus_connection,
            dbus_sensorfw_name,
            dbus_sensorfw_path,
            dbus_sensorfw_interface,
            "releaseSensor",
            g_variant_new("(six)", plugin_string(), m_sessionid, m_pid),
            G_VARIANT_TYPE("(b)"),
            G_DBUS_CALL_FLAGS_NONE,
            timeout_default,
            NULL,
            NULL);

    if (!result)
    {
        log->log(log_tag, "failed to release SensorfwSensor");
        return false;
    }

    gboolean the_result;
    g_variant_get(result, "(b)", &the_result);
    g_variant_unref(result);

    return the_result;
}

void repowerd::Sensorfw::start()
{
    if (m_running)
        return;

    m_running = true;
    read_loop = std::thread([this](){
        log->log(log_tag, "Eventloop started");
        while (m_running) {
            if (m_socket->socket()->waitForReadyRead(10))
                data_recived_impl();
        }
        m_running = false;
        log->log(log_tag, "Eventloop stopped");
    });

    int constexpr timeout_default = 100;
    auto const result =  g_dbus_connection_call_sync(
            dbus_connection,
            dbus_sensorfw_name,
            plugin_path(),
            plugin_interface(),
            "start",
            g_variant_new("(i)", m_sessionid),
            NULL,
            G_DBUS_CALL_FLAGS_NONE,
            timeout_default,
            NULL,
            NULL);

    if (!result)
    {
        log->log(log_tag, "failed to start SensorfwSensor");
        return;
    }
    g_variant_unref(result);
}

void repowerd::Sensorfw::stop()
{
    if (!m_running)
        return;

    m_running = false;

    int constexpr timeout_default = 100;
    auto const result =  g_dbus_connection_call_sync(
            dbus_connection,
            dbus_sensorfw_name,
            plugin_path(),
            plugin_interface(),
            "stop",
            g_variant_new("(i)", m_sessionid),
            NULL,
            G_DBUS_CALL_FLAGS_NONE,
            timeout_default,
            NULL,
            NULL);

    if (!result)
    {
        log->log(log_tag, "failed to stop SensorfwSensor");
    } else {
        g_variant_unref(result);
    }

    read_loop.join();
    read_loop = std::thread();
}

void repowerd::Sensorfw::set_interval(int interval) {
    int constexpr timeout_default = 100;
    auto const result =  g_dbus_connection_call_sync(
            dbus_connection,
            dbus_sensorfw_name,
            plugin_path(),
            plugin_interface(),
            "setInterval",
            g_variant_new("(ii)", m_sessionid, interval),
            NULL,
            G_DBUS_CALL_FLAGS_NONE,
            timeout_default,
            NULL,
            NULL);

    if (!result)
    {
        log->log(log_tag, "set_interval() failed to releaseSensor");
        return;
    }
    g_variant_unref(result);
}
