# Copyright © 2016 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>

include_directories(
    ${ANDROID_PROPERTIES_INCLUDE_DIRS}
    ${GIO_INCLUDE_DIRS}
    ${UA_INCLUDE_DIRS}
)

set(
    REPOWERD_ADAPTER_SRCS

    android_device_config.cpp
    android_device_quirks.cpp
    brightness_params.cpp
    dbus_connection_handle.cpp
    dbus_event_loop.cpp
    dbus_message_handle.cpp
    dev_alarm_wakeup_service.cpp
    event_loop.cpp
    event_loop_timer.cpp
    fd.cpp
    ofono_voice_call_service.cpp
    sysfs_brightness_control.cpp
    ubuntu_proximity_sensor.cpp
    unity_display_power_control.cpp
    unity_power_button.cpp
    unity_screen_service.cpp
    unity_user_activity.cpp
)

add_library(
    repowerd-adapters OBJECT
    ${REPOWERD_ADAPTER_SRCS}
)
