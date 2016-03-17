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

#include "fake_power_button.h"

namespace rt = repowerd::test;

repowerd::HandlerRegistration rt::FakePowerButton::register_power_button_handler(
    PowerButtonHandler const& handler)
{
    mock.register_power_button_handler(handler);
    this->handler = handler;
    return HandlerRegistration{
        [this]
        {
            mock.unregister_power_button_handler();
            this->handler = [](PowerButtonState){};
        }};
}

void rt::FakePowerButton::press()
{
    handler(PowerButtonState::pressed);
}

void rt::FakePowerButton::release()
{
    handler(PowerButtonState::released);
}
