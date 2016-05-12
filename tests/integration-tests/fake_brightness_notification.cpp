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

#include "fake_brightness_notification.h"

namespace rt = repowerd::test;

rt::FakeBrightnessNotification::FakeBrightnessNotification()
    : brightness_handler{[](double){}}
{
}

void rt::FakeBrightnessNotification::emit_brightness(double brightness)
{
    brightness_handler(brightness);
}

repowerd::HandlerRegistration rt::FakeBrightnessNotification::register_brightness_handler(
    repowerd::BrightnessHandler const& handler)
{
    mock.register_brightness_handler(handler);
    this->brightness_handler = handler;
    return repowerd::HandlerRegistration{
        [this]
        {
            mock.unregister_brightness_handler();
            this->brightness_handler = [](double){};
        }};
}
