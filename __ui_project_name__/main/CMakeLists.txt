#Add sources from ui directory
file(GLOB_RECURSE SRC_UI ${CMAKE_SOURCE_DIR} "ui/*.c")

idf_component_register(
    SRCS "main.cpp" ${SRC_UI}
    INCLUDE_DIRS "" "ui"
    REQUIRES LovyanGFX lvgl
)
