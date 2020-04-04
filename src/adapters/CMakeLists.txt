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

add_subdirectory(libsuspend)
add_subdirectory(sensorfw)

set(
    REPOWERD_ADAPTER_SRCS

    android_autobrightness_algorithm.cpp
    android_backlight.cpp
    android_device_config.cpp
    android_device_quirks.cpp
    backlight_brightness_control.cpp
    brightness_params.cpp
    console_log.cpp
    dbus_connection_handle.cpp
    dbus_event_loop.cpp
    dbus_message_handle.cpp
    dev_alarm_wakeup_service.cpp
    default_state_machine_options.cpp
    event_loop.cpp
    event_loop_timer.cpp
    fd.cpp
    libsuspend_system_power_control.cpp
    logind_session_tracker.cpp
    logind_system_power_control.cpp
    monotone_spline.cpp
    null_log.cpp
    ofono_voice_call_service.cpp
    path.cpp
    real_chrono.cpp
    real_filesystem.cpp
    real_temporary_suspend_inhibition.cpp
    repowerd_service.cpp
    syslog_log.cpp
    sysfs_backlight.cpp
    timerfd_wakeup_service.cpp
    ubuntu_light_sensor.cpp
    ubuntu_performance_booster.cpp
    ubuntu_proximity_sensor.cpp
    unity_display.cpp
    unity_power_button.cpp
    unity_screen_service.cpp
    unity_user_activity.cpp
    upower_power_source_and_lid.cpp
)

add_library(
    repowerd-adapters STATIC
    ${REPOWERD_ADAPTER_SRCS}
)

target_link_libraries(
    repowerd-adapters

    suspend
    sensorfw-adapters
    ${DEVICEINFO_LDFLAGS}
    ${GIO_LDFLAGS} ${GIO_LIBRARIES}
    ${GIO_UNIX_LDFLAGS} ${GIO_UNIX_LIBRARIES}
    ${LIBHARDWARE_LDFLAGS} ${LIBHARDWARE_LIBRARIES}
    ${UA_LDFLAGS} ${UA_LIBRARIES}
    ${UBUNTU_PLATFORM_HARDWARE_API_LDFLAGS} ${UBUNTU_PLATFORM_HARDWARE_API_LIBRARIES}
)

target_include_directories(
    repowerd-adapters PUBLIC

    ${DEVICEINFO_INCLUDE_DIRS}
    ${GIO_INCLUDE_DIRS}
    ${GIO_UNIX_INCLUDE_DIRS}
    ${LIBHARDWARE_INCLUDE_DIRS}
    ${UA_INCLUDE_DIRS}
    ${UBUNTU_PLATFORM_HARDWARE_API_INCLUDE_DIRS}
)
