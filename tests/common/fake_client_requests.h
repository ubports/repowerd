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

#include "src/client_requests.h"

#include <gmock/gmock.h>

namespace repowerd
{
namespace test
{

class FakeClientRequests : public ClientRequests
{
public:
    FakeClientRequests();

    HandlerRegistration register_turn_on_display_handler(
        TurnOnDisplayHandler const& handler) override;

    void emit_turn_on_display(TurnOnDisplayTimeout timeout);

    struct Mock
    {
        MOCK_METHOD1(register_turn_on_display_handler, void(TurnOnDisplayHandler const&));
        MOCK_METHOD0(unregister_turn_on_display_handler, void());
    };
    testing::NiceMock<Mock> mock;

private:
    TurnOnDisplayHandler turn_on_display_handler;
};

}
}
