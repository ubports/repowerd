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

project(repowerd)

set(REPOWERD_VERSION "2017.03")

cmake_minimum_required(VERSION 3.0)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option(REPOWERD_BUILD_TESTS "Build tests" ON)
option(REPOWERD_DISABLE_TIME_SENSITIVE_TESTS "Don't run time-sensitive tests" OFF)

# Work around cmake setting conf dir to "/usr/etc" instead of "/etc"
# when prefix is "/usr"
if (NOT DEFINED CMAKE_INSTALL_SYSCONFDIR AND CMAKE_INSTALL_PREFIX STREQUAL "/usr")
    set(CMAKE_INSTALL_SYSCONFDIR "/etc")
endif()

enable_testing()
include(GNUInstallDirs)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(DEVICEINFO REQUIRED deviceinfo)
pkg_check_modules(GIO REQUIRED gio-2.0)
pkg_check_modules(GIO_UNIX REQUIRED gio-unix-2.0)
pkg_check_modules(UA REQUIRED ubuntu-platform-api)
pkg_check_modules(LIBHARDWARE REQUIRED libhardware)
pkg_check_modules(UBUNTU_PLATFORM_HARDWARE_API REQUIRED ubuntu-platform-hardware-api)
pkg_check_modules(OVERLAY_READER REQUIRED liboverlay-reader)
find_package(AndroidHeaders REQUIRED)

set(CMAKE_CXX_STANDARD 14)

set(build_types "None;Debug;Release;RelWithDebInfo;MinSizeRel;AddressSanitizer;ThreadSanitizer")
# Change informational string for CMAKE_BUILD_TYPE
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "${build_types}" FORCE)
# Enable cmake-gui to display a drop down list for CMAKE_BUILD_TYPE
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "${build_types}")

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -pthread -std=c11 -Werror -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -pthread -std=c++14 -Werror -Wall -Wnon-virtual-dtor -Wextra")

string(TOLOWER "${CMAKE_BUILD_TYPE}" cmake_build_type_lower)

if(cmake_build_type_lower MATCHES "addresssanitizer")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fsanitize=address")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
elseif(cmake_build_type_lower MATCHES "threadsanitizer")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread -fno-omit-frame-pointer")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fsanitize=thread")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=thread")
    link_libraries(tsan) # Workaround for LP:1413474
endif()

set(POWERD_DEVICE_CONFIG_DIR "${CMAKE_INSTALL_FULL_DATAROOTDIR}/powerd/device_configs")
set(REPOWERD_DEVICE_CONFIG_DIR "${CMAKE_INSTALL_FULL_DATAROOTDIR}/repowerd/device-configs")

add_definitions(-DREPOWERD_VERSION="${REPOWERD_VERSION}")

add_subdirectory(src/)
add_subdirectory(data/)

if(REPOWERD_BUILD_TESTS)
    find_package(GtestGmock REQUIRED)
    add_subdirectory(tests/)
endif()
