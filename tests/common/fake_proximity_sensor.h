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

#include "src/core/proximity_sensor.h"

#include <gmock/gmock.h>

namespace repowerd
{
namespace test
{

class FakeProximitySensor : public ProximitySensor
{
public:
    FakeProximitySensor();

    HandlerRegistration register_proximity_handler(
        ProximityHandler const& handler) override;
    ProximityState proximity_state() override;

    void enable_proximity_events() override;
    void disable_proximity_events() override;

    void emit_proximity_state(ProximityState state);
    void emit_proximity_state_if_enabled(ProximityState state);
    void set_proximity_state(ProximityState state);

    struct Mock
    {
        MOCK_METHOD1(register_proximity_handler, void(ProximityHandler const&));
        MOCK_METHOD0(unregister_proximity_handler, void());
    };
    testing::NiceMock<Mock> mock;

private:
    bool events_enabled;
    ProximityHandler handler;
    ProximityState state;
};

}
}
