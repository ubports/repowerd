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

#include "core/daemon.h"
#include "core/log.h"
#include "default_daemon_config.h"

#include <csignal>
#include <cstring>

namespace
{

char const* const log_tag = "main";

struct SignalHandler
{
    SignalHandler(repowerd::Daemon* daemon_ptr, repowerd::Log* log_ptr)
    {
        SignalHandler::daemon_ptr = daemon_ptr;
        SignalHandler::log_ptr = log_ptr;

        struct sigaction new_action;
        new_action.sa_handler = stop_daemon;
        new_action.sa_flags = 0;
        sigfillset(&new_action.sa_mask);

        sigaction(SIGINT, &new_action, &old_sigint_action);
        sigaction(SIGTERM, &new_action, &old_sigterm_action);
    }

    ~SignalHandler()
    {
        sigaction(SIGINT, &old_sigint_action, nullptr);
        sigaction(SIGTERM, &old_sigterm_action, nullptr);
    }

    static void stop_daemon(int sig)
    {
        log_ptr->log(log_tag, "Received signal %s", strsignal(sig));
        daemon_ptr->stop();
    }

    static repowerd::Daemon* daemon_ptr;
    static repowerd::Log* log_ptr;

    struct sigaction old_sigint_action;
    struct sigaction old_sigterm_action;
};

repowerd::Daemon* SignalHandler::daemon_ptr{nullptr};
repowerd::Log* SignalHandler::log_ptr{nullptr};

}

int main()
{
    repowerd::DefaultDaemonConfig config;
    auto const log = config.the_log();

    log->log(log_tag, "Starting repowerd");

    repowerd::Daemon daemon{config};
    SignalHandler signal_handler{&daemon, log.get()};

    daemon.run();

    log->log(log_tag, "Exiting repowerd");
}
