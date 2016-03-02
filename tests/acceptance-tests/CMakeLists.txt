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
    ${CMAKE_SOURCE_DIR}
)


add_executable(
    repowerd-acceptance-tests

    test_power_button.cpp

    daemon_config.cpp
    fake_power_button.cpp

    $<TARGET_OBJECTS:repowerd-objects>
)

target_link_libraries(
    repowerd-acceptance-tests

    ${GTEST_BOTH_LIBRARIES}
    ${GMOCK_LIBRARY}
    ${GMOCK_MAIN_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
)

add_test(repowerd-acceptance-tests ${EXECUTABLE_OUTPUT_PATH}/repowerd-acceptance-tests)

add_dependencies(repowerd-acceptance-tests GMock)
