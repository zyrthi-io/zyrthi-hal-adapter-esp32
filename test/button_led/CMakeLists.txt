cmake_minimum_required(VERSION 3.16)

get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

set(EXTRA_COMPONENT_DIRS 
    "${PROJECT_ROOT}"
    "${PROJECT_ROOT}/components"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(button_led)
