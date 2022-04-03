cmake_minimum_required(VERSION 3.5)
project(durchblick VERSION 0.1.2)


set(PLUGIN_AUTHOR "univrsal")
set(PLUGIN_GIT durchblick)
set(LINUX_MAINTAINER_EMAIL "uni@vrsal.xyz")
set(MACOS_BUNDLEID "xyz.vrsal.durchblick")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_PREFIX_PATH "${QTDIR}")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

string(TIMESTAMP TODAY "%Y.%m.%d %H:%M")
add_definitions(-DBUILD_TIME="${TODAY}")
add_definitions(-DPLUGIN_VERSION="${PROJECT_VERSION}")

if (MSVC OR APPLE)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
endif()

find_package(LibObs REQUIRED)
find_package(Qt5Widgets REQUIRED)

option(LOCAL_INSTALLATION "Whether to install for the current user (default: OFF)" OFF)
option(GLOBAL_INSTALLATION "Whether to install for all users (default: OFF)" OFF)


if (OBS_FRONTEND_INCLUDE)
    include_directories(${OBS_FRONTEND_INCLUDE})
endif()

configure_file(
    package/installer-macOS.pkgproj.in
    ../package/installer-macOS.pkgproj
)
configure_file(
    package/installer-Windows.iss.in
    ../package/installer-Windows.iss
)
configure_file(
    package/README.txt.in
    ../package/README.txt
)

set(durchblick_ui
        )

qt5_wrap_ui(durchblick_ui_headers ${durchblick_ui})

if (UNIX AND NOT APPLE)
    set(LIBOBS_LIBRARIES "libobs")
    set(OBS_FRONTEND_LIB "obs-frontend-api")
endif()

if (UNIX)
    find_package(Qt5Gui REQUIRED)
    include_directories(${Qt5Gui_PRIVATE_INCLUDE_DIRS})
    set(durchblick_platform_deps
        Qt5::Gui
        )
endif()

if (APPLE)
    add_definitions(-DMACOS=1)
    find_library(COCOA Cocoa)

    include_directories(${COCOA})

    set(FRONTEND_LIB "${OBS_FRONTEND_LIB}")
else()
    set(FRONTEND_LIB "obs-frontend-api")
    set(OBS_LIB "libobs")
endif()

if (MSVC)
    set(durchblick_platform_source
	./src/windows-helper.hpp
	./src/windows-helper.cpp)
    set(durchblick_platform_deps
        "${LibObs_DIR}/../deps/jansson/lib/RelWithDebInfo/jansson.lib"
        ${W32_PTHREADS_LIB})
endif()


set(durchblick_sources
    ./src/plugin.cpp
    ./src/durchblick.hpp
    ./src/durchblick.cpp
    ./src/layout.hpp
    ./src/layout.cpp
    ./src/qt_display.hpp
    ./src/qt_display.cpp
    ./src/new_item_dialog.cpp
    ./src/new_item_dialog.hpp
    ./src/layout_config_dialog.cpp
    ./src/layout_config_dialog.hpp
    ./src/registry.cpp
    ./src/registry.hpp
    ./src/item.hpp
    ./src/config.cpp
    ./src/config.hpp
    ./src/scene_item.cpp
    ./src/scene_item.hpp
    ./src/source_item.cpp
    ./src/source_item.hpp
    ./src/preview_program_item.cpp
    ./src/preview_program_item.hpp
    ./src/custom_item.cpp
    ./src/custom_item.hpp
    ./src/callbacks.h
    ${durchblick_platform_source}
    ${durchblick_ui_headers}
)

add_library(durchblick MODULE
    ${durchblick_sources}
    ${durchblick_ui}
    ${durchblick_platform_sources})

if (MSVC)
    target_compile_definitions(durchblick PRIVATE UNICODE _UNICODE)
    set(platform_includes "${LibObs_DIR}/../deps/jansson/include")
endif()

include_directories(
    "${LIBOBS_INCLUDE_DIR}/../UI/obs-frontend-api"
    ${LIBOBS_INCLUDE_DIR}
    ${Qt5Core_INCLUDES}
    ${Qt5Widgets_INCLUDES}
    ${platform_includes}
)

target_link_libraries(durchblick
    ${LIBOBS_LIBRARIES}
    Qt5::Widgets
    Qt5::Core
    Qt5::Gui
    ${OBS_FRONTEND_LIB}
    ${durchblick_platform_deps})

# --- Windows-specific build settings and tasks ---
if(WIN32)
    if(NOT DEFINED OBS_FRONTEND_LIB)
        set(OBS_FRONTEND_LIB "OBS_FRONTEND_LIB-NOTFOUND" CACHE FILEPATH "OBS frontend library")
        message(FATAL_ERROR "Could not find OBS Frontend API\'s library !")
    endif()

    # Enable Multicore Builds and disable FH4 (to not depend on VCRUNTIME140_1.DLL when building with VS2019)
    if (MSVC)
        add_definitions(/MP /d2FH4-)
    endif()

    include_directories(
        "${LIBOBS_INCLUDE_DIR}"
    )

    target_link_libraries(${CMAKE_PROJECT_NAME}
        "${OBS_FRONTEND_LIB}"
    )

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(ARCH_NAME "64bit")
        set(OBS_BUILDDIR_ARCH "build64")
    else()
        set(ARCH_NAME "32bit")
        set(OBS_BUILDDIR_ARCH "build32")
    endif()

    get_filename_component(LibOBS_DIR ${LIBOBS_LIB} DIRECTORY)

    add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        # Copy to obs-studio dev environment for immediate testing
        COMMAND if $<CONFIG:Debug>==1 (
            "${CMAKE_COMMAND}" -E copy
                "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
                "${LibOBS_DIR}/../../rundir/$<CONFIG>/obs-plugins/${ARCH_NAME}")

        COMMAND if $<CONFIG:Debug>==1 (
            "${CMAKE_COMMAND}" -E copy
                "$<TARGET_PDB_FILE:${CMAKE_PROJECT_NAME}>"
                "${LibOBS_DIR}/../../rundir/$<CONFIG>/obs-plugins/${ARCH_NAME}")

        COMMAND if $<CONFIG:Debug>==1 (
            "${CMAKE_COMMAND}" -E make_directory
                "${LibOBS_DIR}/../../rundir/$<CONFIG>/data/obs-plugins/${CMAKE_PROJECT_NAME}")

        COMMAND if $<CONFIG:Debug>==1 (
            "${CMAKE_COMMAND}" -E copy_directory
                "${PROJECT_SOURCE_DIR}/data"
                "${LibOBS_DIR}/../../rundir/$<CONFIG>/data/obs-plugins/${CMAKE_PROJECT_NAME}")
    )

elseif(UNIX AND NOT APPLE)
    include(GNUInstallDirs)

    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES PREFIX "")
    target_link_libraries(${CMAKE_PROJECT_NAME} obs-frontend-api)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(OBS_ARCH_NAME "64bit")
        set(OBS_BUILDDIR_ARCH "build64")
    else()
        set(OBS_ARCH_NAME "32bit")
        set(OBS_BUILDDIR_ARCH "build32")
    endif()
    if (LOCAL_INSTALLATION)
        # Installs into home directory
        install(TARGETS ${CMAKE_PROJECT_NAME}
            LIBRARY DESTINATION "$ENV{HOME}/.config/obs-studio/plugins/${CMAKE_PROJECT_NAME}/bin/${OBS_ARCH_NAME}")

        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data
            DESTINATION "$ENV{HOME}/.config/obs-studio/plugins/${CMAKE_PROJECT_NAME}")
    elseif(GLOBAL_INSTALLATION)
        # For *.deb installer
        install(TARGETS ${CMAKE_PROJECT_NAME}
            LIBRARY DESTINATION "/usr/lib/obs-plugins") # hardcoded, since ubuntu uses the wrong path by default

        file(GLOB locale_files data/locale/*.ini)
        file(GLOB other_files data/*.*)

        install(FILES ${locale_files}
            DESTINATION "${CMAKE_INSTALL_FULL_DATAROOTDIR}/obs/obs-plugins/${CMAKE_PROJECT_NAME}/locale")
        install(FILES ${other_files}
            DESTINATION "${CMAKE_INSTALL_FULL_DATAROOTDIR}/obs/obs-plugins/${CMAKE_PROJECT_NAME}")
    else()
        # For *.zip binary
        install(TARGETS ${CMAKE_PROJECT_NAME}
            LIBRARY DESTINATION "${CMAKE_PROJECT_NAME}/bin/${OBS_ARCH_NAME}")

        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data
            DESTINATION "${CMAKE_PROJECT_NAME}")

        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/package/README.txt"
            DESTINATION "${CMAKE_PROJECT_NAME}")
    endif()
elseif(APPLE)
    if(NOT DEFINED OBS_FRONTEND_LIB)
        set(OBS_FRONTEND_LIB "OBS_FRONTEND_LIB-NOTFOUND" CACHE FILEPATH "OBS frontend library")
        message(FATAL_ERROR "Could not find OBS Frontend API\'s library !")
    endif()

    target_link_libraries(${CMAKE_PROJECT_NAME}
        "${OBS_FRONTEND_LIB}"
    )

    if (LOCAL_INSTALLATION)
        # Installs into home directory
        install(TARGETS ${CMAKE_PROJECT_NAME}
            LIBRARY DESTINATION "$ENV{HOME}/Library/Application Support/obs-studio/plugins/${CMAKE_PROJECT_NAME}/bin/")

        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data
            DESTINATION "$ENV{HOME}/Library/Application Support/obs-studio/plugins/${CMAKE_PROJECT_NAME}")
    endif()
endif()
