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

add_executable(
    repowerd-proximity-tool

    proximity_tool.cpp

    $<TARGET_OBJECTS:repowerd-adapters>
    $<TARGET_OBJECTS:repowerd-core>
)

target_link_libraries(
    repowerd-proximity-tool

    ${ANDROID_PROPERTIES_LDFLAGS} ${ANDROID_PROPERTIES_LIBRARIES}
    ${GIO_LDFLAGS} ${GIO_LIBRARIES}
    ${UA_LDFLAGS} ${UA_LIBRARIES}
)
