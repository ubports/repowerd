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

#include "src/core/power_source.h"

#include <gmock/gmock.h>

namespace repowerd
{
namespace test
{

class FakePowerSource : public PowerSource
{
public:
    void start_processing() override;

    HandlerRegistration register_power_source_change_handler(
        PowerSourceChangeHandler const& handler) override;

    void emit_power_source_change();

    struct Mock
    {
        MOCK_METHOD0(start_processing, void());
        MOCK_METHOD1(register_power_source_change_handler, void(PowerSourceChangeHandler const&));
        MOCK_METHOD0(unregister_power_source_change_handler, void());
    };
    testing::NiceMock<Mock> mock;

private:
    PowerSourceChangeHandler power_source_change_handler;
};

}
}
