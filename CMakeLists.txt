cmake_minimum_required(VERSION 4.0)
project(SplinterCellPatch)

set(CMAKE_CXX_STANDARD 26)

# Build as shared library (DLL)
add_library(SplinterCellPatch SHARED src/library.cpp)

# Add Detours include directory
target_include_directories(SplinterCellPatch PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Compiler flags for MSVC
if(MSVC)
    # Warning level 4 (highest practical warning level, /Wall is too noisy on MSVC)
    target_compile_options(SplinterCellPatch PRIVATE /W4)

    # Treat warnings as errors
    target_compile_options(SplinterCellPatch PRIVATE /WX)

    # Additional useful warnings
    target_compile_options(SplinterCellPatch PRIVATE
        /w14640  # Enable warning on thread-unsafe static member initialization
        /w14265  # Class has virtual functions but destructor is not virtual
        /w14263  # Member function does not override any base class virtual member function
    )

    # Release-specific optimizations
    target_compile_options(SplinterCellPatch PRIVATE
        $<$<CONFIG:Release>:/O2>      # Maximum optimization (speed)
        $<$<CONFIG:Release>:/Oi>      # Enable intrinsic functions
        $<$<CONFIG:Release>:/Ot>      # Favor fast code
        $<$<CONFIG:Release>:/GL>      # Whole program optimization
    )

    # Release-specific linker flags
    target_link_options(SplinterCellPatch PRIVATE
        $<$<CONFIG:Release>:/LTCG>           # Link-time code generation
        $<$<CONFIG:Release>:/OPT:REF>        # Eliminate unreferenced functions/data
        $<$<CONFIG:Release>:/OPT:ICF>        # Identical COMDAT folding
        $<$<CONFIG:Release>:/DEBUG:NONE>     # Strip debug information
    )
endif()

# Link appropriate Detours library based on architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # 64-bit build
    target_link_libraries(SplinterCellPatch PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/detours_x64.lib)
else()
    # 32-bit build
    target_link_libraries(SplinterCellPatch PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/detours_x86.lib)
endif()
