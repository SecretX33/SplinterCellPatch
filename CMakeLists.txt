cmake_minimum_required(VERSION 4.0)
project(SplinterCellPatch)

set(CMAKE_CXX_STANDARD 26)

# Build as shared library (DLL)
add_library(SplinterCellPatch SHARED src/library.cpp)

# Find and link Microsoft Detours
# User should have Detours installed or provide path via CMAKE_PREFIX_PATH
find_path(DETOURS_INCLUDE_DIR detours.h)
find_library(DETOURS_LIBRARY NAMES detours)

if(DETOURS_INCLUDE_DIR AND DETOURS_LIBRARY)
    target_include_directories(SplinterCellPatch PRIVATE ${DETOURS_INCLUDE_DIR})
    target_link_libraries(SplinterCellPatch PRIVATE ${DETOURS_LIBRARY})
else()
    message(WARNING "Detours not found. Install Detours or set CMAKE_PREFIX_PATH")
endif()
