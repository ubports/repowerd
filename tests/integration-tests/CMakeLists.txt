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

pkg_check_modules(FUSE REQUIRED fuse)

include_directories(
    ${FUSE_INCLUDE_DIRS}
)

add_definitions(${FUSE_CFLAGS})

add_executable(
    repowerd-integration-tests

    dbus_bus.cpp
    dbus_client.cpp
    fake_android_properties.cpp
    fake_brightness_notification.cpp
    fake_device_config.cpp
    fake_libhardware.cpp
    fake_ofono.cpp
    fake_upower.cpp
    fake_wakeup_service.cpp
    run_command.cpp
    temporary_environment_value.cpp
    temporary_file.cpp
    unity_screen_dbus_client.cpp
    virtual_filesystem.cpp

    test_android_backlight.cpp
    test_android_autobrightness_algorithm.cpp
    test_android_device_config.cpp
    test_dev_alarm_wakeup_service.cpp
    test_event_loop_timer.cpp
    test_ofono_voice_call_service.cpp
    test_powerd_service.cpp
    test_backlight_brightness_control.cpp
    test_path.cpp
    test_sysfs_backlight.cpp
    test_ubuntu_light_sensor.cpp
    test_ubuntu_proximity_sensor.cpp
    test_unity_display_power_control.cpp
    test_unity_power_button.cpp
    test_unity_screen_service.cpp
    test_unity_user_activity.cpp
    test_upower_power_source.cpp
    test_virtual_filesystem.cpp
)

target_link_libraries(
    repowerd-integration-tests

    repowerd-core
    repowerd-adapters
    repowerd-test-common

    ${GTEST_BOTH_LIBRARIES}
    ${GMOCK_LIBRARY}
    ${GMOCK_MAIN_LIBRARY}
    ${FUSE_LDFLAGS} ${FUSE_LIBRARIES}
)

if (REPOWERD_DISABLE_FUSE_INTEGRATION_TESTS)
    set(INTEGRATION_TESTS_FILTER "${INTEGRATION_TESTS_FILTER}:AnAndroidDeviceConfig.*:ABacklightBrightnessControl.*:ADevAlarmWakeupService.*:APath.*:AVirtualFilesystem.*")
endif()

if (REPOWERD_DISABLE_TIME_SENSITIVE_INTEGRATION_TESTS)
    set(INTEGRATION_TESTS_FILTER "${INTEGRATION_TESTS_FILTER}:ABacklightBrightnessControl.*:ADevAlarmWakeupService.*:AnEventLoopTimer.*")
endif()

add_test(
    repowerd-integration-tests
    ${EXECUTABLE_OUTPUT_PATH}/repowerd-integration-tests --gtest_filter=-${INTEGRATION_TESTS_FILTER}
)

add_dependencies(repowerd-integration-tests GMock)
