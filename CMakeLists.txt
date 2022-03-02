cmake_minimum_required(VERSION 3.4)
cmake_policy(SET CMP0048 NEW)

project(buttons LANGUAGES C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(UV_USE_STATIC_LIBS TRUE)
find_package(uv REQUIRED)

file(GLOB_RECURSE
        SOURCE_FILES
        example.c
        buttons.c
        common/common_buttons.c
        )

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

include_directories(
        ${CMAKE_SOURCE_DIR}
        ${UV_INCLUDE_DIR}
        )

target_link_libraries( ${PROJECT_NAME}
        ${UV_LIBRARY}
        pthread
        dl
        )

