/*
 * Copyright © 2017 Canonical Ltd.
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

#include "fake_display_information.h"

namespace rt = repowerd::test;

rt::FakeDisplayInformation::FakeDisplayInformation()
    : has_active_external_displays_{false}
{
}

bool rt::FakeDisplayInformation::has_active_external_displays()
{
    return has_active_external_displays_;
}

void rt::FakeDisplayInformation::set_has_active_external_displays(bool b)
{
    has_active_external_displays_ = b;
}
