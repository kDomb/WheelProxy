
set(SRCS main.cpp device.cpp map.cpp axis.cpp sourcedevice.cpp
    targetdevice.cpp button.cpp eventsource.cpp mapentry.cpp
    utils.cpp application.cpp
    )


add_executable(${PROJECT_NAME} ${SRCS})
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

 
