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

#pragma once

#include "src/power_button.h"

namespace repowerd
{
namespace test
{

class FakePowerButton : public PowerButton
{
public:
    PowerButtonHandlerId register_power_button_handler(PowerButtonHandler const& handler) override;
    void unregister_power_button_handler(PowerButtonHandlerId id) override;

    void press();
    void release();

private:
    PowerButtonHandler handler;
};

}
}
