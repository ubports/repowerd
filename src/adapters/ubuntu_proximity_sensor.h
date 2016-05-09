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
#include "event_loop.h"

#include <memory>
#include <mutex>
#include <condition_variable>
#include <vector>

#include <ubuntu/application/sensors/proximity.h>

namespace repowerd
{

class DeviceQuirks;

class UbuntuProximitySensor : public ProximitySensor
{
public:
    UbuntuProximitySensor(DeviceQuirks const& device_quirks);

    HandlerRegistration register_proximity_handler(
        ProximityHandler const& handler) override;
    ProximityState proximity_state() override;

    void enable_proximity_events() override;
    void disable_proximity_events() override;

private:
    enum class EnablementMode{with_handler, without_handler};

    static void static_sensor_reading_callback(UASProximityEvent* event, void* context);
    void handle_proximity_event(ProximityState state);
    void enable_proximity_events_unqueued(EnablementMode mode);
    void disable_proximity_events_unqueued(EnablementMode mode);
    ProximityState wait_for_valid_state();
    void schedule_synthetic_far_event();
    void invalidate_synthetic_far_event();

    bool is_enabled();
    bool should_invoke_handler();

    UASensorsProximity* const sensor;
    EventLoop event_loop;

    std::vector<EnablementMode> enablements;

    ProximityHandler handler;
    bool handler_enabled;
    int synthetic_event_seqno;
    std::chrono::milliseconds synthetic_event_delay;

    std::mutex state_mutex;
    std::condition_variable state_cv;
    bool is_state_valid;
    ProximityState state;
};

}
