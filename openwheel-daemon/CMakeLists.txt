cmake_minimum_required(VERSION 3.0)
project(openwheel-daemon C)

set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED dbus-1)
include_directories(..)

add_executable(asus_wheel
        hidreader.c
        openwheel.h
        helpers.h
        helpers.c)


target_include_directories(asus_wheel PRIVATE ${DBUS_INCLUDE_DIRS})

target_link_libraries(asus_wheel PRIVATE ${DBUS_LIBRARIES})

target_compile_options(asus_wheel PRIVATE ${DBUS_CFLAGS_OTHER})

enable_testing()

add_executable(test_find_hidraw_device
        tests/test_find_hidraw_device.c
        helpers.c)

target_include_directories(test_find_hidraw_device PRIVATE ${DBUS_INCLUDE_DIRS})
target_link_libraries(test_find_hidraw_device PRIVATE ${DBUS_LIBRARIES})
target_compile_options(test_find_hidraw_device PRIVATE ${DBUS_CFLAGS_OTHER})

add_test(NAME test_find_hidraw_device COMMAND test_find_hidraw_device)