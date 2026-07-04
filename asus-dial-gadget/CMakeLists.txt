cmake_minimum_required(VERSION 3.16)
project(asus-dial-gadget CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets Qml Quick DBus Test)
find_package(PkgConfig REQUIRED)
pkg_check_modules(XTST REQUIRED xtst)
pkg_check_modules(X11 REQUIRED x11)

qt_standard_project_setup()

qt_add_library(asus-dial-gadget-core STATIC
    src/DialFunction.h
    src/FunctionRegistry.h
    src/FunctionRegistry.cpp
    src/ProcessRunner.h
    src/ProcessRunner.cpp
    src/DBusListener.h
    src/DBusListener.cpp
    src/DialController.h
    src/DialController.cpp
    src/RotateDispatcher.h
    src/SyncRotateDispatcher.h
    src/SyncRotateDispatcher.cpp
    src/FunctionWorker.h
    src/FunctionWorker.cpp
    src/AsyncRotateDispatcher.h
    src/AsyncRotateDispatcher.cpp
    src/TrayController.h
    src/TrayController.cpp
    src/TrayIcon.h
    src/TrayIcon.cpp
    src/DBusCaller.h
    src/QtDBusCaller.h
    src/QtDBusCaller.cpp
    src/LogindSession.h
    src/LogindSession.cpp
    src/Backlight.h
    src/Backlight.cpp
    src/BacklightReader.h
    src/BacklightReader.cpp
    src/functions/VolumeFunction.h
    src/functions/VolumeFunction.cpp
    src/functions/BrightnessFunction.h
    src/functions/BrightnessFunction.cpp
    src/functions/MediaFunction.h
    src/functions/MediaFunction.cpp
    src/functions/ScrollBackend.h
    src/functions/ScrollFunction.h
    src/functions/ScrollFunction.cpp
    src/functions/X11ScrollBackend.h
    src/functions/X11ScrollBackend.cpp
    src/functions/UinputScrollBackend.h
    src/functions/UinputScrollBackend.cpp
    src/functions/ScrollBackendFactory.h
    src/functions/ScrollBackendFactory.cpp
)
target_link_libraries(asus-dial-gadget-core PUBLIC
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Qml Qt6::DBus
)
target_link_libraries(asus-dial-gadget-core PRIVATE ${XTST_LIBRARIES} ${X11_LIBRARIES})
target_include_directories(asus-dial-gadget-core PUBLIC src)
target_include_directories(asus-dial-gadget-core PRIVATE ${XTST_INCLUDE_DIRS} ${X11_INCLUDE_DIRS})

qt_add_executable(asus-dial-gadget src/main.cpp)
target_link_libraries(asus-dial-gadget PRIVATE asus-dial-gadget-core Qt6::Quick)

qt_add_qml_module(asus-dial-gadget
    URI AsusDialGadget
    VERSION 1.0
    QML_FILES
        qml/DialOverlay.qml
        qml/RadialMenu.qml
        qml/Hud.qml
        qml/CenterLogo.qml
        qml/DialIcon.qml
    RESOURCE_PREFIX /qt/qml
)

enable_testing()
add_subdirectory(tests)