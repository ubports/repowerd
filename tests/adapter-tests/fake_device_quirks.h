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

#include "src/adapters/device_quirks.h"

namespace repowerd
{
namespace test
{

struct FakeDeviceQuirks : repowerd::DeviceQuirks
{
    FakeDeviceQuirks();

    std::chrono::milliseconds synthetic_initial_proximity_event_delay() const override;
    DeviceQuirks::ProximityEventType synthetic_initial_proximity_event_type() const override;
    bool normal_before_display_on_autobrightness() const override;
    bool ignore_session_deactivation() const override;

    void set_synthetic_initial_event_type_near();
    void set_normal_before_display_on_autobrightness(bool value);
    void set_ignore_session_deactivation(bool value);

private:
    repowerd::DeviceQuirks::ProximityEventType synthetic_initial_event_type_;
    bool normal_before_display_on_autobrightness_;
    bool ignore_session_deactivation_;
};

}
}
