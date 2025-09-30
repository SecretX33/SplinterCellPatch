cmake_minimum_required(VERSION 4.0)
project(SplinterCellPatch)

set(CMAKE_CXX_STANDARD 26)

# Build as shared library (DLL)
add_library(SplinterCellPatch SHARED src/library.cpp)

# Find and link Microsoft Detours
find_package(Detours CONFIG REQUIRED)
target_link_libraries(SplinterCellPatch PRIVATE Microsoft.Detours::Detours)
