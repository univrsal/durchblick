cmake_minimum_required(VERSION 3.16...3.21)

# Change obs-plugintemplate to your plugin's name in a machine-readable format (e.g.:
# obs-myawesomeplugin) and set
project(durchblick VERSION 0.4.0)
add_library(${CMAKE_PROJECT_NAME} MODULE)

string(TIMESTAMP TODAY "%Y.%m.%d %H:%M")
add_definitions(-DBUILD_TIME="${TODAY}")
add_definitions(-DSCRAB_VERSION="${PROJECT_VERSION}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/external")

# Replace `Your Name Here` with the name (yours or your organization's) you want to see as the
# author of the plugin (in the plugin's metadata itself and in the installers)
set(PLUGIN_AUTHOR "univrsal")

# Replace `com.example.obs-plugin-template` with a unique Bundle ID for macOS releases (used both in
# the installer and when submitting the installer for notarization)
set(MACOS_BUNDLEID "xyz.vrsal.${CMAKE_PROJECT_NAME}")

# Replace `me@contoso.com` with the maintainer email address you want to put in Linux packages
set(LINUX_MAINTAINER_EMAIL "uni@vrsal.xyz")

option(LOCAL_INSTALLATION "Whether to install for the current user (default: OFF)" OFF)
if (MSVC)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        ./src/util/windows_helper.hpp
        ./src/util/windows_helper.cpp)
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
)

# Import libobs as main plugin dependency
include(cmake/ObsPluginHelpers.cmake)

# Uncomment these lines if you want to use the OBS Frontend API in your plugin

if (LOCAL_INSTALLATION)

    if (UNIX AND NOT APPLE)
        target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "/usr/include/obs")
        set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
        find_package(LibObs REQUIRED)
        target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE obs-frontend-api libobs)

        # Installs into home directory
        add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
            # Copy to obs-studio dev environment for immediate testing
            COMMAND "${CMAKE_COMMAND}" -E make_directory
                    "$ENV{HOME}/.config/obs-studio/plugins/${CMAKE_PROJECT_NAME}/bin/64bit"
            COMMAND "${CMAKE_COMMAND}" -E make_directory
                    "$ENV{HOME}/.config/obs-studio/plugins/${CMAKE_PROJECT_NAME}/data"
            COMMAND "${CMAKE_COMMAND}" -E copy
                    "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
                    "$ENV{HOME}/.config/obs-studio/plugins/${CMAKE_PROJECT_NAME}/bin/64bit"
            COMMAND "${CMAKE_COMMAND}" -E copy_directory
                    "${PROJECT_SOURCE_DIR}/data"
                    "$ENV{HOME}/.config/obs-studio/plugins/${CMAKE_PROJECT_NAME}/data"
        )
    elseif (WIN32)
        # Enable Multicore Builds and disable FH4 (to not depend on VCRUNTIME140_1.DLL when building with VS2019)
        if (MSVC)
            add_definitions(/MP /d2FH4-)
        endif()

        add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
            # Copy to obs-studio dev environment for immediate testing
            COMMAND if $<CONFIG:Debug>==1 (
                "${CMAKE_COMMAND}" -E copy
                    "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
                    "${libobs_DIR}/../rundir/$<CONFIG>/obs-plugins/64bit")
            COMMAND if $<CONFIG:Debug>==1 (
                "${CMAKE_COMMAND}" -E copy
                    "$<TARGET_PDB_FILE:${CMAKE_PROJECT_NAME}>"
                    "${libobs_DIR}/../rundir/$<CONFIG>/obs-plugins/64bit")
            COMMAND if $<CONFIG:Debug>==1 (
                "${CMAKE_COMMAND}" -E make_directory
                    "${libobs_DIR}/../rundir/$<CONFIG>/data/obs-plugins/${CMAKE_PROJECT_NAME}")
            COMMAND if $<CONFIG:Debug>==1 (
                "${CMAKE_COMMAND}" -E copy_directory
                    "${PROJECT_SOURCE_DIR}/data"
                    "${libobs_DIR}/../rundir/$<CONFIG>/data/obs-plugins/${CMAKE_PROJECT_NAME}")
        )
    endif()
endif()

if (WIN32 OR APPLE)
    find_package(jansson REQUIRED)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE jansson)
endif()

find_package(libobs REQUIRED)
find_package(obs-frontend-api REQUIRED)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

# Uncomment those lines if you want to use Qt in your plugin
find_qt(COMPONENTS Widgets Core Gui)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt::Core Qt::Widgets Qt::Gui)
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${Qt6Gui_INCLUDES})

configure_file(src/plugin-macros.h.in ${CMAKE_SOURCE_DIR}/src/plugin-macros.generated.h)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/plugin-macros.generated.h)

# /!\ TAKE NOTE: No need to edit things past this point /!\

# --- Platform-independent build settings ---

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)


# --- End of section ---

# --- Windows-specific build settings and tasks ---
if(OS_WINDOWS)
  configure_file(cmake/bundle/windows/installer-Windows.iss.in
                 ${CMAKE_BINARY_DIR}/installer-Windows.generated.iss)

  if(MSVC)
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE /W4)
  endif()
  # --- End of section ---

  # -- macOS specific build settings and tasks --
elseif(OS_MACOS)
  configure_file(cmake/bundle/macos/installer-macos.pkgproj.in
                 ${CMAKE_BINARY_DIR}/installer-macos.generated.pkgproj)

  set(MACOSX_PLUGIN_GUI_IDENTIFIER "${MACOS_BUNDLEID}")
  set(MACOSX_PLUGIN_BUNDLE_VERSION "${CMAKE_PROJECT_VERSION}")
  set(MACOSX_PLUGIN_SHORT_VERSION_STRING "1")

  target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall)
  # --- End of section ---

  # --- Linux-specific build settings and tasks ---
else()
  target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall)
endif()
# --- End of section ---

setup_plugin_target(${CMAKE_PROJECT_NAME})
