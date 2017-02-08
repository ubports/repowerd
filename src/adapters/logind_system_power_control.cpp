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

#include "logind_system_power_control.h"
#include "scoped_g_error.h"
#include "event_loop_handler_registration.h"

#include "src/core/log.h"

#include <gio/gio.h>
#include <gio/gunixfdlist.h>

namespace
{
char const* const log_tag = "LogindSystemPowerControl";

char const* const dbus_logind_name = "org.freedesktop.login1";
char const* const dbus_manager_path = "/org/freedesktop/login1";
char const* const dbus_manager_interface = "org.freedesktop.login1.Manager";
}

repowerd::LogindSystemPowerControl::LogindSystemPowerControl(
    std::shared_ptr<Log> const& log,
    std::string const& dbus_bus_address)
    : log{log},
      dbus_connection{dbus_bus_address},
      dbus_event_loop{"SystemPower"},
      system_resume_handler{[]{}},
      idle_and_lid_inhibition_fd{-1}
{
}

void repowerd::LogindSystemPowerControl::start_processing()
{
    dbus_manager_signal_handler_registration = dbus_event_loop.register_signal_handler(
        dbus_connection,
        dbus_logind_name,
        dbus_manager_interface,
        "PrepareForSleep",
        dbus_manager_path,
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
}

repowerd::HandlerRegistration
repowerd::LogindSystemPowerControl::register_system_resume_handler(
    SystemResumeHandler const& handler)
{
    return EventLoopHandlerRegistration{
        dbus_event_loop,
        [this, &handler] { this->system_resume_handler = handler; },
        [this] { this->system_resume_handler = []{}; }};
}

void repowerd::LogindSystemPowerControl::allow_suspend(
    std::string const& id, SuspendType suspend_type)
{
    if (suspend_type != SuspendType::any)
        return;

    std::unique_lock<std::mutex> lock{inhibitions_mutex};

    log->log(log_tag, "releasing inhibition for %s", id.c_str());

    suspend_disallowances.erase(id);

    if (!pending_suspends.empty())
    {
        lock.unlock();
        suspend_if_allowed();
    }
}

void repowerd::LogindSystemPowerControl::disallow_suspend(
    std::string const& id, SuspendType suspend_type)
{
    if (suspend_type != SuspendType::any)
        return;

    auto inhibition_fd = dbus_inhibit("sleep:idle", id.c_str());

    std::lock_guard<std::mutex> lock{inhibitions_mutex};
    suspend_disallowances.emplace(id, std::move(inhibition_fd));
}

void repowerd::LogindSystemPowerControl::power_off()
{
    dbus_power_off();
}

void repowerd::LogindSystemPowerControl::suspend_if_allowed()
{
    auto const allowed_to_suspend =
        [this]
        {
            std::lock_guard<std::mutex> lock{inhibitions_mutex};
            return suspend_disallowances.empty();
        };

    if (allowed_to_suspend())
        dbus_suspend();
}

void repowerd::LogindSystemPowerControl::suspend_when_allowed(std::string const& id)
{
    {
        std::lock_guard<std::mutex> lock{inhibitions_mutex};
        pending_suspends.insert(id);
    }

    suspend_if_allowed();
}

void repowerd::LogindSystemPowerControl::cancel_suspend_when_allowed(std::string const& id)
{
    std::lock_guard<std::mutex> lock{inhibitions_mutex};

    pending_suspends.erase(id);
}

void repowerd::LogindSystemPowerControl::allow_default_system_handlers()
{
    std::lock_guard<std::mutex> lock{inhibitions_mutex};

    log->log(log_tag, "releasing idle and lid inhibition");

    idle_and_lid_inhibition_fd = Fd{-1};
}

void repowerd::LogindSystemPowerControl::disallow_default_system_handlers()
{
    auto inhibition_fd = dbus_inhibit("idle:handle-lid-switch", "repowerd handles idle and lid");

    std::lock_guard<std::mutex> lock{inhibitions_mutex};
    idle_and_lid_inhibition_fd = std::move(inhibition_fd);
}

void repowerd::LogindSystemPowerControl::handle_dbus_signal(
    GDBusConnection* /*connection*/,
    gchar const* /*sender*/,
    gchar const* /*object_path*/,
    gchar const* /*interface_name*/,
    gchar const* signal_name_cstr,
    GVariant* parameters)
{
    std::string const signal_name{signal_name_cstr ? signal_name_cstr : ""};

    if (signal_name == "PrepareForSleep")
    {
        gboolean start{FALSE};
        g_variant_get(parameters, "(b)", &start);

        log->log(log_tag, "dbus_PrepareForSleep(%s)", start ? "true" : "false");

        if (start == FALSE)
            system_resume_handler();
    }
}


repowerd::Fd repowerd::LogindSystemPowerControl::dbus_inhibit(
    char const* what, char const* why)
{
    int constexpr timeout_default = -1;
    auto constexpr null_cancellable = nullptr;
    GUnixFDList *fd_list = nullptr;
    ScopedGError error;

    char const* const who = "repowerd";
    char const* const mode = "block";

    log->log(log_tag, "dbus_inhibit(%s,%s)...", what, why);

    auto const result = g_dbus_connection_call_with_unix_fd_list_sync(
        dbus_connection,
        dbus_logind_name,
        dbus_manager_path,
        dbus_manager_interface,
        "Inhibit",
        g_variant_new("(ssss)", what, who, why, mode),
        G_VARIANT_TYPE("(h)"),
        G_DBUS_CALL_FLAGS_NONE,
        timeout_default,
        nullptr,
        &fd_list,
        null_cancellable,
        error);

    if (!result)
    {
        log->log(log_tag, "dbus_inhibit() failed: %s", error.message_str().c_str());
        return Fd{-1};
    }

    gint32 inhibit_fd_index = -1;
    g_variant_get(result, "(h)", &inhibit_fd_index);
    g_variant_unref(result);

    auto inhibit_fd = Fd{g_unix_fd_list_get(fd_list, inhibit_fd_index, error)};
    if (inhibit_fd < 0)
        log->log(log_tag, "dbus_inhibit() get fd failed: %s", error.message_str().c_str());
    else
        log->log(log_tag, "dbus_inhibit(%s,%s) done", what, why);
    g_object_unref(fd_list);

    return inhibit_fd;
}

void repowerd::LogindSystemPowerControl::dbus_power_off()
{
    int constexpr timeout_default = -1;
    auto constexpr null_cancellable = nullptr;
    ScopedGError error;

    log->log(log_tag, "dbus_power_off()...");

    auto const result = g_dbus_connection_call_sync(
        dbus_connection,
        dbus_logind_name,
        dbus_manager_path,
        dbus_manager_interface,
        "PowerOff",
        g_variant_new("(b)", FALSE),
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        timeout_default,
        null_cancellable,
        error);

    if (!result)
        log->log(log_tag, "dbus_power_off() failed: %s", error.message_str().c_str());
    else
        log->log(log_tag, "dbus_power_off() done");

    g_variant_unref(result);
}

void repowerd::LogindSystemPowerControl::dbus_suspend()
{
    int constexpr timeout_default = -1;
    auto constexpr null_cancellable = nullptr;
    ScopedGError error;

    log->log(log_tag, "dbus_suspend()...");

    auto const result = g_dbus_connection_call_sync(
        dbus_connection,
        dbus_logind_name,
        dbus_manager_path,
        dbus_manager_interface,
        "Suspend",
        g_variant_new("(b)", FALSE),
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        timeout_default,
        null_cancellable,
        error);

    if (!result)
        log->log(log_tag, "dbus_suspend() failed: %s", error.message_str().c_str());
    else
        log->log(log_tag, "dbus_suspend() done");

    g_variant_unref(result);
}
