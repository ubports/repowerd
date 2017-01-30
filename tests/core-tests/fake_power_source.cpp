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

#include "fake_power_source.h"

namespace rt = repowerd::test;

void rt::FakePowerSource::start_processing()
{
    mock.start_processing();
}

repowerd::HandlerRegistration rt::FakePowerSource::register_power_source_change_handler(
    PowerSourceChangeHandler const& handler)
{
    mock.register_power_source_change_handler(handler);
    this->power_source_change_handler = handler;
    return HandlerRegistration{
        [this]
        {
            mock.unregister_power_source_change_handler();
            this->power_source_change_handler = []{};
        }};
}

repowerd::HandlerRegistration rt::FakePowerSource::register_power_source_critical_handler(
    PowerSourceChangeHandler const& handler)
{
    mock.register_power_source_critical_handler(handler);
    this->power_source_critical_handler = handler;
    return HandlerRegistration{
        [this]
        {
            mock.unregister_power_source_critical_handler();
            this->power_source_critical_handler = []{};
        }};
}

bool rt::FakePowerSource::is_using_battery_power()
{
    return is_using_battery_power_;
}

void rt::FakePowerSource::emit_power_source_change()
{
    power_source_change_handler();
}

void rt::FakePowerSource::emit_power_source_critical()
{
    power_source_critical_handler();
}

void rt::FakePowerSource::set_is_using_battery_power(bool b)
{
    is_using_battery_power_ = b;
}
