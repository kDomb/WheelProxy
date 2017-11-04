

set(CONFIG_FILES
    F1_2017_Thrustmaster_Italia_458_to_Logitech_G920.json
    Dirt_Rally_Thrustmaster_Italia_458_to_Logitech_Driving_Force_GT.json)


# show files in qt creator
add_custom_target(Config SOURCES ${CONFIG_FILES})


INSTALL(FILES ${CONFIG_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/configs/)


