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

#include "run_daemon.h"
#include "src/core/daemon.h"

std::thread repowerd::test::run_daemon(Daemon& daemon)
{
    auto daemon_thread = std::thread{[&] { daemon.run(); }};

    // Flush twice to ensure the daemon has started up fully. The first flush
    // ensures the daemon has entered the main loop. The second flush ensures
    // that any startup events (e.g. from FakeSessionTracker to set the active
    // session) have been processed.
    daemon.flush();
    daemon.flush();

    return daemon_thread;
}
