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

#include "src/default_daemon_config.h"
#include "src/core/session_tracker.h"

#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <string>

int main()
{
    setenv("REPOWERD_LOG", "console", 1);

    repowerd::DefaultDaemonConfig config;
    auto const session_tracker = config.the_session_tracker();

    auto const active_reg = session_tracker->register_active_session_changed_handler(
        [] (std::string const& session_id, repowerd::SessionType type)
        {
            auto const compatibility_cstr = 
                (type == repowerd::SessionType::RepowerdCompatible) ?
                "compatible" :
                "incompatible";
            std::cout << "ACTIVE SESSION: " << session_id
                      << " (" << compatibility_cstr << " with repowerd)" << std::endl;
        });

    auto const remove_reg = session_tracker->register_session_removed_handler(
        [] (std::string const& session_id)
        {
            std::cout << "SESSION REMOVED: " << session_id << std::endl;
        });

    session_tracker->start_processing();

    pause();
}
