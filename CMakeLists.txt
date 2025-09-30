cmake_minimum_required(VERSION 4.0)
project(SplinterCellPatch)

set(CMAKE_CXX_STANDARD 26)

# Build as shared library (DLL)
add_library(SplinterCellPatch SHARED src/library.cpp)

# Add Detours include directory
target_include_directories(SplinterCellPatch PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link appropriate Detours library based on architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # 64-bit build
    target_link_libraries(SplinterCellPatch PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/detours_x64.lib)
else()
    # 32-bit build
    target_link_libraries(SplinterCellPatch PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/detours_x86.lib)
endif()
