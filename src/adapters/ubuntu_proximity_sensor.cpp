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

#include "ubuntu_proximity_sensor.h"
#include "device_quirks.h"
#include "event_loop_handler_registration.h"

#include "src/core/log.h"

#include <stdexcept>
#include <algorithm>

namespace
{

char const* const log_tag = "UbuntuProximitySensor";
auto const null_handler = [](repowerd::ProximityState){};

char const* proximity_state_to_cstr(repowerd::ProximityState state)
{
    return state == repowerd::ProximityState::far ? "far" : "near";
}

}

repowerd::UbuntuProximitySensor::UbuntuProximitySensor(
    std::shared_ptr<Log> const& log,
    DeviceQuirks const& device_quirks)
    : log{log},
      sensor{ua_sensors_proximity_new()},
      handler{null_handler},
      synthetic_event_seqno{1},
      synthetic_event_delay{device_quirks.synthetic_initial_proximity_event_delay()},
      synthetic_event_state{
          device_quirks.synthetic_initial_proximity_event_type() ==
              DeviceQuirks::ProximityEventType::far ?
                  ProximityState::far : ProximityState::near},
      is_state_valid{false},
      state{ProximityState::far}
{
    if (!sensor)
        throw std::runtime_error("Failed to allocate proximity sensor");

    ua_sensors_proximity_set_reading_cb(sensor, static_sensor_reading_callback, this);
}

repowerd::HandlerRegistration repowerd::UbuntuProximitySensor::register_proximity_handler(
    ProximityHandler const& handler)
{
    return EventLoopHandlerRegistration{
        event_loop,
        [this, &handler]{ this->handler = handler; },
        [this]{ this->handler = null_handler; }};
}

repowerd::ProximityState repowerd::UbuntuProximitySensor::proximity_state()
{
    log->log(log_tag, "proximity_state()");

    event_loop.enqueue(
        [this]
        {
            enable_proximity_events_unqueued(EnablementMode::without_handler);
        }).get();

    auto const valid_state = wait_for_valid_state();

    event_loop.enqueue(
        [this]
        {
            disable_proximity_events_unqueued(EnablementMode::without_handler);
        });

    log->log(log_tag, "proximity_state() => %s",
             proximity_state_to_cstr(valid_state));

    return valid_state;
}

void repowerd::UbuntuProximitySensor::enable_proximity_events()
{
    log->log(log_tag, "enable_proximity_events()");

    event_loop.enqueue(
        [this]
        {
            enable_proximity_events_unqueued(EnablementMode::with_handler);
        }).get();
}

void repowerd::UbuntuProximitySensor::disable_proximity_events()
{
    log->log(log_tag, "disable_proximity_events()");

    event_loop.enqueue(
        [this]
        {
            disable_proximity_events_unqueued(EnablementMode::with_handler);
        }).get();
}

void repowerd::UbuntuProximitySensor::static_sensor_reading_callback(
    UASProximityEvent* event, void* context)
{
    auto const ups = static_cast<UbuntuProximitySensor*>(context);
    auto const distance = uas_proximity_event_get_distance(event);

    auto const state = (distance == U_PROXIMITY_NEAR) ?
                       ProximityState::near : ProximityState::far;

    ups->event_loop.enqueue([ups, state] { ups->handle_proximity_event(state); });
}

void repowerd::UbuntuProximitySensor::handle_proximity_event(ProximityState new_state)
{
    log->log(log_tag, "handle_proximity_event(%s)",
             proximity_state_to_cstr(new_state));

    invalidate_synthetic_initial_event();

    {
        std::lock_guard<std::mutex> lock{state_mutex};
        state = new_state;
        is_state_valid = true;
        state_cv.notify_all();
    }

    if (should_invoke_handler())
        handler(state);
}

void repowerd::UbuntuProximitySensor::enable_proximity_events_unqueued(
    EnablementMode mode)
{
    if (!is_enabled())
    {
        ua_sensors_proximity_enable(sensor);
        schedule_synthetic_initial_event();
    }

    enablements.push_back(mode);
}

void repowerd::UbuntuProximitySensor::disable_proximity_events_unqueued(
    EnablementMode mode)
{
    if (is_enabled())
    {
        auto const iter = std::find(
            enablements.begin(), enablements.end(), mode);
        if (iter != enablements.end())
            enablements.erase(iter);

        if (!is_enabled())
        {
            ua_sensors_proximity_disable(sensor);
            std::lock_guard<std::mutex> lock{state_mutex};
            is_state_valid = false;
        }
    }
}

repowerd::ProximityState repowerd::UbuntuProximitySensor::wait_for_valid_state()
{
    std::unique_lock<std::mutex> lock{state_mutex};

    state_cv.wait(
        lock,
        [this] { return is_state_valid; });

    return state;
}

void repowerd::UbuntuProximitySensor::schedule_synthetic_initial_event()
{
    if (synthetic_event_delay.count() < 0 ||
        synthetic_event_delay == std::chrono::milliseconds::max())
    {
        return;
    }

    log->log(log_tag, "schedule_synthetic_initial_event(), state=%s, delay_ms=%d",
             proximity_state_to_cstr(synthetic_event_state),
             static_cast<int>(synthetic_event_delay.count()));

    // Some proximity sensors occasionally don't send an initial event when
    // enabled. Work around this by sending a synthetic initial event if no
    // events have been emitted "soon" after enabling the sensor.
    event_loop.schedule_in(synthetic_event_delay,
        [this, expected_seqno = synthetic_event_seqno]
        {
            if (synthetic_event_seqno == expected_seqno)
            {
                log->log(log_tag, "emitting synthetic initial event %s",
                         proximity_state_to_cstr(synthetic_event_state));
                handle_proximity_event(synthetic_event_state);
            }
        });
}

void repowerd::UbuntuProximitySensor::invalidate_synthetic_initial_event()
{
    ++synthetic_event_seqno;
}

bool repowerd::UbuntuProximitySensor::is_enabled()
{
    return !enablements.empty();
}

bool repowerd::UbuntuProximitySensor::should_invoke_handler()
{
    for (auto const& e : enablements)
        if (e == EnablementMode::with_handler) return true;

    return false;
}
