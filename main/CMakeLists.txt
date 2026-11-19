# Sources are collected at configure time: after adding or removing a .c, run
# `bash .claude/idf-build.sh reconfigure build`.
file(GLOB_RECURSE SRCS "main.c" "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

idf_component_register(SRCS ${SRCS}
                       INCLUDE_DIRS "." "stepper" "braille_pin" "button"
                       PRIV_REQUIRES esp_driver_gpio esp_driver_ledc esp_rom freertos log)

# board_config.h (pins and hardware constants) is visible in every translation unit
target_compile_options(${COMPONENT_LIB} PRIVATE -include ${CMAKE_CURRENT_SOURCE_DIR}/board_config.h)
