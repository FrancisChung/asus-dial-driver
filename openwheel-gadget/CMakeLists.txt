cmake_minimum_required(VERSION 3.16)
project(openwheel-gadget CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets Qml Quick DBus Test)
find_package(PkgConfig REQUIRED)
pkg_check_modules(XTST REQUIRED xtst)

qt_standard_project_setup()

qt_add_library(openwheel-gadget-core STATIC
    src/DialFunction.h
    src/FunctionRegistry.h
    src/FunctionRegistry.cpp
    src/ProcessRunner.h
    src/ProcessRunner.cpp
    src/DBusCaller.h
    src/QtDBusCaller.h
    src/QtDBusCaller.cpp
    src/LogindSession.h
    src/LogindSession.cpp
    src/Backlight.h
    src/Backlight.cpp
    src/functions/VolumeFunction.h
    src/functions/VolumeFunction.cpp
    src/functions/BrightnessFunction.h
    src/functions/BrightnessFunction.cpp
    src/functions/MediaFunction.h
    src/functions/MediaFunction.cpp
)
target_link_libraries(openwheel-gadget-core PUBLIC
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Qml Qt6::DBus
)
target_link_libraries(openwheel-gadget-core PRIVATE ${XTST_LIBRARIES})
target_include_directories(openwheel-gadget-core PUBLIC src)
target_include_directories(openwheel-gadget-core PRIVATE ${XTST_INCLUDE_DIRS})

qt_add_executable(openwheel-gadget src/main.cpp)
target_link_libraries(openwheel-gadget PRIVATE openwheel-gadget-core Qt6::Quick)

qt_add_qml_module(openwheel-gadget
    URI OpenWheelGadget
    VERSION 1.0
    QML_FILES
        qml/DialOverlay.qml
    RESOURCE_PREFIX /qt/qml
)

enable_testing()
add_subdirectory(tests)