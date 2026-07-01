qt_add_executable(test_functionregistry test_functionregistry.cpp)
target_link_libraries(test_functionregistry PRIVATE openwheel-gadget-core Qt6::Test)
add_test(NAME FunctionRegistryTest COMMAND test_functionregistry)

qt_add_executable(test_volumefunction test_volumefunction.cpp)
target_link_libraries(test_volumefunction PRIVATE openwheel-gadget-core Qt6::Test)
add_test(NAME VolumeFunctionTest COMMAND test_volumefunction)

qt_add_executable(test_brightnessfunction test_brightnessfunction.cpp)
target_link_libraries(test_brightnessfunction PRIVATE openwheel-gadget-core Qt6::Test Qt6::DBus)
add_test(NAME BrightnessFunctionTest COMMAND test_brightnessfunction)

qt_add_executable(test_logindsession test_logindsession.cpp)
target_link_libraries(test_logindsession PRIVATE openwheel-gadget-core Qt6::Test Qt6::DBus)
add_test(NAME LogindSessionTest COMMAND test_logindsession)

qt_add_executable(test_mediafunction test_mediafunction.cpp)
target_link_libraries(test_mediafunction PRIVATE openwheel-gadget-core Qt6::Test Qt6::DBus)
add_test(NAME MediaFunctionTest COMMAND test_mediafunction)

qt_add_executable(test_scrollfunction test_scrollfunction.cpp)
target_link_libraries(test_scrollfunction PRIVATE openwheel-gadget-core Qt6::Test)
add_test(NAME ScrollFunctionTest COMMAND test_scrollfunction)

qt_add_executable(test_dbuslistener test_dbuslistener.cpp)
target_link_libraries(test_dbuslistener PRIVATE openwheel-gadget-core Qt6::Test Qt6::DBus)
add_test(NAME DBusListenerTest COMMAND test_dbuslistener)

qt_add_executable(test_dialcontroller test_dialcontroller.cpp)
target_link_libraries(test_dialcontroller PRIVATE openwheel-gadget-core Qt6::Test)
add_test(NAME DialControllerTest COMMAND test_dialcontroller)

qt_add_executable(test_traycontroller test_traycontroller.cpp)
target_link_libraries(test_traycontroller PRIVATE openwheel-gadget-core Qt6::Test)
add_test(NAME TrayControllerTest COMMAND test_traycontroller)
