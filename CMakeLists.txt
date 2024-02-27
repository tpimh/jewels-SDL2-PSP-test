cmake_minimum_required(VERSION 3.10)

project(jewels)

add_executable(${PROJECT_NAME} main.c)

include(FindPkgConfig)
pkg_search_module(SDL2 REQUIRED sdl2)

find_package(Freetype REQUIRED)
find_package(Harfbuzz REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${SDL2_LIBRARIES}
    Freetype::Freetype
    harfbuzz::harfbuzz
)

IF(PLATFORM_PSP)
    file(COPY platform/psp/ICON0.PNG DESTINATION ${CMAKE_BINARY_DIR})

    create_pbp_file(
        TARGET ${PROJECT_NAME}
        ICON_PATH ICON0.PNG
        BACKGROUND_PATH NULL
        PREVIEW_PATH NULL
        TITLE ${PROJECT_NAME}
    )
ELSEIF(VITA)
    set(VITA_TITLEID "JWLT00001")

    include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

    vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME})
    vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
        VERSION "01.00"
        NAME ${PROJECT_NAME}
        FILE platform/vita/sce_sys sce_sys
        FILE fonts/DejaVuSerif.ttf fonts/DejaVuSerif.ttf
        FILE fonts/amiri-0.104/amiri-regular.ttf fonts/amiri-0.104/amiri-regular.ttf
        FILE fonts/fireflysung-1.3.0/fireflysung.ttf fonts/fireflysung-1.3.0/fireflysung.ttf
    )
ENDIF()

if(NOT DEFINED VITA)
        file(COPY fonts/DejaVuSerif.ttf DESTINATION ${CMAKE_BINARY_DIR}/fonts)
        file(COPY fonts/amiri-0.104/amiri-regular.ttf DESTINATION ${CMAKE_BINARY_DIR}/fonts/amiri-0.104)
        file(COPY fonts/fireflysung-1.3.0/fireflysung.ttf DESTINATION ${CMAKE_BINARY_DIR}/fonts/fireflysung-1.3.0)
endif()
