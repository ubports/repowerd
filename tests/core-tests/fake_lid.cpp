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

#include "fake_lid.h"

namespace rt = repowerd::test;

rt::FakeLid::FakeLid()
    : lid_handler{[](auto){}}
{
}

void rt::FakeLid::start_processing()
{
    mock.start_processing();
}

repowerd::HandlerRegistration rt::FakeLid::register_lid_handler(
    LidHandler const& handler)
{
    mock.register_lid_handler(handler);
    this->lid_handler = handler;
    return HandlerRegistration{
        [this]
        {
            mock.unregister_lid_handler();
            this->lid_handler = [](auto){};
        }};
}

void rt::FakeLid::close()
{
    lid_handler(LidState::closed);
}

void rt::FakeLid::open()
{
    lid_handler(LidState::open);
}
