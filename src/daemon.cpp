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

#include "daemon.h"
#include "power_button.h"
#include "display_power_control.h"

repowerd::Daemon::Daemon(DaemonConfig& config)
    : power_button{config.the_power_button()},
      display_power_control{config.the_display_power_control()},
      done_future{done_promise.get_future()},
      is_display_on{false}
{
}

void repowerd::Daemon::run(std::promise<void>& started)
{
    auto const handler_id = power_button->register_power_button_handler(
        [this] (PowerButtonState state)
        { 
            if (state == PowerButtonState::pressed)
            {
                if (is_display_on)
                    display_power_control->turn_off();
                else
                    display_power_control->turn_on();
            }
        });

    started.set_value();
    done_future.wait();

    power_button->unregister_power_button_handler(handler_id);
}

void repowerd::Daemon::stop()
{
    done_promise.set_value();
}
