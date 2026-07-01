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
