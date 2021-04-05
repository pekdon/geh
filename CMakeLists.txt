cmake_minimum_required(VERSION 3.5)

project(geh)
set(geh_VERSION_MAJOR 0)
set(geh_VERSION_MINOR 4)
set(geh_VERSION_PATCH 0)
set(geh_VERSION "${geh_VERSION_MAJOR}.${geh_VERSION_MINOR}.${geh_VERSION_PATCH}")

option(PREFER_GTK2 "Build with GTK2 even if GTK3 is available" OFF)

# Look for dependencies
find_package(PkgConfig)
pkg_check_modules(GTK3 gtk+-3.0)

if (PREFER_GTK2 OR NOT GTK3_FOUND)
  find_package(GTK2)
endif (PREFER_GTK2 OR NOT GTK3_FOUND)

if (GTK2_FOUND)
  set(GTK_INCLUDE_DIRS ${GTK2_INCLUDE_DIRS})
  set(GTK_LIBRARIES ${GTK2_LIBRARIES})
  set(UI_TOOLKIT=GTK2)
elseif (GTK3_FOUND)
  set(GTK_INCLUDE_DIRS ${GTK3_INCLUDE_DIRS})
  set(GTK_LIBRARIES ${GTK3_LIBRARIES})
  set(UI_TOOLKIT=GTK2)
else (GTK2_FOUND)
  message(FATAL_ERROR "Either GTK2 or GTK3 is required" )
endif (GTK2_FOUND)

# Subdirectories
add_subdirectory(src)
