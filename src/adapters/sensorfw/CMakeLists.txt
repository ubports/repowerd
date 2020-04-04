set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)

add_library(sensorfw-adapters STATIC
    sensorfw_common.cpp
    sensorfw_light_sensor.cpp
    sensorfw_proximity_sensor.cpp

    socketreader.cpp
)

target_link_libraries(sensorfw-adapters PUBLIC
    Qt5::Core
    Qt5::Network
)

target_include_directories(sensorfw-adapters PUBLIC
    ${GIO_INCLUDE_DIRS}
    "../"
)
