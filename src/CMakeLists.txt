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

add_subdirectory(core)
add_subdirectory(adapters)
add_subdirectory(tools)

add_executable(
    repowerd

    default_daemon_config.cpp
    main.cpp
)

target_link_libraries(
    repowerd

    repowerd-core
    repowerd-adapters
)


install(
  TARGETS repowerd
  RUNTIME DESTINATION bin
)
