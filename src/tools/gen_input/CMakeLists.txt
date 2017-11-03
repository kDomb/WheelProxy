set(SRCS main.cpp)

add_executable(${PROJECT_NAME}GenInput ${SRCS})
install(TARGETS ${PROJECT_NAME}GenInput RUNTIME DESTINATION bin)

