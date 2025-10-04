cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" ON)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

string(TIMESTAMP TODAY "%Y.%m.%d %H:%M")

# Get the current working branch
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE)

# Get the latest commit hash
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE)

target_compile_definitions(${PROJECT_NAME} PUBLIC
    GIT_BRANCH="${GIT_BRANCH}"
    GIT_COMMIT_HASH="${GIT_COMMIT_HASH}"
    PLUGIN_VERSION="${_version}"
    BUILD_TIME="${TODAY}"
)

add_subdirectory(deps/jansson)
if(MSVC)
  target_compile_options(jansson PRIVATE /wd4267)
endif()

find_package(libobs REQUIRED)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs jansson)
include_directories(${CMAKE_BINARY_DIR}/deps/jansson/include)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

if (MSVC)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        ./src/util/windows_helper.cpp
        ./src/util/windows_helper.hpp
    )
endif()

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ./src/durchblick_plugin.cpp
    ./src/layout.hpp
    ./src/layout.cpp
    ./src/config.cpp
    ./src/config.hpp
    ./src/util/util.h
    ./src/util/callbacks.h
    ./src/util/platform_util.hpp
    ./src/util/display_helpers.hpp
    ./src/util/volume_meter.cpp
    ./src/util/volume_meter.hpp
    ./src/util/mixer_renderer.cpp
    ./src/util/mixer_renderer.hpp
    ./src/ui/durchblick_dock.hpp
    ./src/ui/durchblick_dock.cpp
    ./src/ui/durchblick.hpp
    ./src/ui/durchblick.cpp
    ./src/ui/qt_display.hpp
    ./src/ui/qt_display.cpp
    ./src/ui/new_item_dialog.cpp
    ./src/ui/new_item_dialog.hpp
    ./src/ui/layout_config_dialog.cpp
    ./src/ui/layout_config_dialog.hpp
    ./src/items/registry.cpp
    ./src/items/registry.hpp
    ./src/items/item.hpp
    ./src/items/scene_item.cpp
    ./src/items/scene_item.hpp
    ./src/items/source_item.cpp
    ./src/items/source_item.hpp
    ./src/items/preview_program_item.cpp
    ./src/items/preview_program_item.hpp
    ./src/items/custom_item.cpp
    ./src/items/custom_item.hpp
    ./src/items/audio_mixer.cpp
    ./src/items/audio_mixer.hpp
)


set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})
