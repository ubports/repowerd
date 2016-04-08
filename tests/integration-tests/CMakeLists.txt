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
    ${GIO_INCLUDE_DIRS}
    ${UA_INCLUDE_DIRS}
)

add_executable(
    repowerd-integration-tests

    dbus_bus.cpp
    dbus_client.cpp
    run_command.cpp
    unity_screen_dbus_client.cpp

    test_ubuntu_proximity_sensor.cpp
    test_unity_screen_service.cpp

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
    ${GIO_LDFLAGS} ${GIO_LIBRARIES}
    ${UA_LDFLAGS} ${UA_LIBRARIES}
)

add_test(repowerd-integration-tests ${EXECUTABLE_OUTPUT_PATH}/repowerd-integration-tests)

add_dependencies(repowerd-integration-tests GMock)
