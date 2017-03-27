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

char const* const suspend_id = "LogindSystemPowerControl";

auto null_arg_handler = []{};
auto null_arg1_handler = [](auto){};
}

repowerd::LogindSystemPowerControl::LogindSystemPowerControl(
    std::shared_ptr<Log> const& log,
    std::string const& dbus_bus_address)
    : log{log},
      dbus_connection{dbus_bus_address},
      dbus_event_loop{"SystemPower"},
      system_resume_handler{null_arg_handler},
      system_allow_suspend_handler{null_arg1_handler},
      system_disallow_suspend_handler{null_arg1_handler},
      is_suspend_blocked{false},
      idle_and_lid_inhibition_fd{-1}
{
}

void repowerd::LogindSystemPowerControl::start_processing()
{
    auto const dbus_signal_handler =
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
        };

    dbus_manager_signal_handler_registration = dbus_event_loop.register_signal_handler(
        dbus_connection,
        dbus_logind_name,
        dbus_manager_interface,
        "PrepareForSleep",
        dbus_manager_path,
        dbus_signal_handler);

    dbus_manager_properties_handler_registration = dbus_event_loop.register_signal_handler(
        dbus_connection,
        dbus_logind_name,
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        dbus_manager_path,
        dbus_signal_handler);

    dbus_event_loop.enqueue([this] { initialize_is_suspend_blocked(); }).get();
}

repowerd::HandlerRegistration
repowerd::LogindSystemPowerControl::register_system_resume_handler(
    SystemResumeHandler const& handler)
{
    return EventLoopHandlerRegistration{
        dbus_event_loop,
        [this, &handler] { this->system_resume_handler = handler; },
        [this] { this->system_resume_handler = null_arg_handler; }};
}

repowerd::HandlerRegistration
repowerd::LogindSystemPowerControl::register_system_allow_suspend_handler(
    SystemAllowSuspendHandler const& handler)
{
    return EventLoopHandlerRegistration{
        dbus_event_loop,
        [this, &handler] { this->system_allow_suspend_handler = handler; },
        [this] { this->system_allow_suspend_handler = null_arg1_handler; }};
}

repowerd::HandlerRegistration
repowerd::LogindSystemPowerControl::register_system_disallow_suspend_handler(
    SystemDisallowSuspendHandler const& handler)
{
    return EventLoopHandlerRegistration{
        dbus_event_loop,
        [this, &handler] { this->system_disallow_suspend_handler = handler; },
        [this] { this->system_disallow_suspend_handler = null_arg1_handler; }};
}

void repowerd::LogindSystemPowerControl::allow_automatic_suspend(std::string const&)
{
}

void repowerd::LogindSystemPowerControl::disallow_automatic_suspend(std::string const&)
{
}

void repowerd::LogindSystemPowerControl::power_off()
{
    dbus_power_off();
}

void repowerd::LogindSystemPowerControl::suspend()
{
    dbus_suspend();
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
    else if (signal_name == "PropertiesChanged")
    {
        char const* properties_interface_cstr{""};
        GVariantIter* properties_iter;
        g_variant_get(parameters, "(&sa{sv}as)",
                      &properties_interface_cstr, &properties_iter, nullptr);

        std::string const properties_interface{properties_interface_cstr};

        if (properties_interface == "org.freedesktop.login1.Manager")
            handle_dbus_change_manager_properties(properties_iter);

        g_variant_iter_free(properties_iter);
    }
}

void repowerd::LogindSystemPowerControl::handle_dbus_change_manager_properties(
    GVariantIter* properties_iter)
{
    char const* key_cstr{""};
    GVariant* value{nullptr};

    while (g_variant_iter_next(properties_iter, "{&sv}", &key_cstr, &value))
    {
        auto const key_str = std::string{key_cstr};

        if (key_str == "BlockInhibited")
        {
            // TODO: logind incorrectly returns the previous property value,
            // so we need to get it manually
            auto const blocks = dbus_get_block_inhibited();
            log->log(log_tag, "change_manager_properties(), BlockInhibited=%s",
                     blocks.c_str());

            auto const prev_is_suspend_blocked = is_suspend_blocked;
            update_suspend_block(blocks);
            if (is_suspend_blocked != prev_is_suspend_blocked)
                notify_suspend_block_state();
        }

        g_variant_unref(value);
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

void repowerd::LogindSystemPowerControl::initialize_is_suspend_blocked()
{
    auto const blocks = dbus_get_block_inhibited();
    log->log(log_tag, "initialize_is_suspend_blocked(), BlockInhibited=%s",
             blocks.c_str());
    update_suspend_block(blocks);
    notify_suspend_block_state();
}

std::string repowerd::LogindSystemPowerControl::dbus_get_block_inhibited()
{
    int constexpr timeout_default = -1;
    auto constexpr null_cancellable = nullptr;
    ScopedGError error;

    auto const result = g_dbus_connection_call_sync(
        dbus_connection,
        dbus_logind_name,
        dbus_manager_path,
        "org.freedesktop.DBus.Properties",
        "Get",
        g_variant_new("(ss)", dbus_manager_interface, "BlockInhibited"),
        G_VARIANT_TYPE("(v)"),
        G_DBUS_CALL_FLAGS_NONE,
        timeout_default,
        null_cancellable,
        error);

    if (!result)
    {
        log->log(log_tag, "dbus_get_block_inhibited() failed to get BlockInhibited: %s",
                 error.message_str().c_str());
        return "";
    }

    GVariant* block_inhibited_variant{nullptr};
    g_variant_get(result, "(v)", &block_inhibited_variant);

    char const* blocks_cstr{""};
    g_variant_get(block_inhibited_variant, "&s", &blocks_cstr);

    std::string const blocks{blocks_cstr};

    g_variant_unref(block_inhibited_variant);
    g_variant_unref(result);

    return blocks;
}

void repowerd::LogindSystemPowerControl::update_suspend_block(std::string const& blocks)
{
    is_suspend_blocked = blocks.find("sleep") != std::string::npos;
}

void repowerd::LogindSystemPowerControl::notify_suspend_block_state()
{
    if (is_suspend_blocked)
        system_disallow_suspend_handler(suspend_id);
    else
        system_allow_suspend_handler(suspend_id);
}
