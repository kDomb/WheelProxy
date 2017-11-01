cmake_minimum_required(VERSION 2.8)
project(WheelProxy)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_CXX_FLAGS "-O3")
set(APPARMOR "ON")
set(SECCOMP "ON")



if(NOT SECCOMP)
add_definitions(-DNO_SECCOMP)
endif(NOT SECCOMP)

add_subdirectory(src)
add_subdirectory(config)

if(APPARMOR)
    set(BIN ${CMAKE_INSTALL_PREFIX}/bin/${PROJECT_NAME})
    configure_file(apparmor/apparmor.in apparmor)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/apparmor DESTINATION /etc/apparmor.d RENAME wheelproxy)
endif(APPARMOR)




# uninstall target
if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()
