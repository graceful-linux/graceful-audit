cmake_minimum_required(VERSION 3.0)
project(graceful-audit C CXX)

set(CMAKE_CXX_STANDARD 17)

set(IS_UOS false)
set(IS_KYLIN false)

set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)

set(CMAKE_INCLUDE_CURRENT_DIR OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-w -g)

find_package(PkgConfig)
#pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(GLIB REQUIRED glib-2.0 gio-2.0)

add_subdirectory(daemon)