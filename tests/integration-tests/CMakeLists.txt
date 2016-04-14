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
    ${GIO_INCLUDE_DIRS}
    ${UA_INCLUDE_DIRS}
)

add_definitions(${FUSE_CFLAGS})

add_executable(
    repowerd-integration-tests

    dbus_bus.cpp
    dbus_client.cpp
    run_command.cpp
    unity_screen_dbus_client.cpp
    virtual_filesystem.cpp

    test_event_loop_timer.cpp
    test_sysfs_brightness_control.cpp
    test_ubuntu_proximity_sensor.cpp
    test_unity_display_power_control.cpp
    test_unity_power_button.cpp
    test_unity_screen_service.cpp
    test_unity_user_activity.cpp
    test_virtual_filesystem.cpp

    $<TARGET_OBJECTS:repowerd-adapters>
    $<TARGET_OBJECTS:repowerd-core>
    $<TARGET_OBJECTS:repowerd-test-common>
)

target_link_libraries(
    repowerd-integration-tests

    ${GTEST_BOTH_LIBRARIES}
    ${GMOCK_LIBRARY}
    ${GMOCK_MAIN_LIBRARY}
    ${ANDROID_PROPERTIES_LDFLAGS} ${ANDROID_PROPERTIES_LIBRARIES}
    ${FUSE_LDFLAGS} ${FUSE_LIBRARIES}
    ${GIO_LDFLAGS} ${GIO_LIBRARIES}
    ${UA_LDFLAGS} ${UA_LIBRARIES}
)

add_test(repowerd-integration-tests ${EXECUTABLE_OUTPUT_PATH}/repowerd-integration-tests)

add_dependencies(repowerd-integration-tests GMock)
