# Find fmt library
# This module defines:
# FMT_FOUND - system has fmt
# FMT_INCLUDE_DIRS - fmt include directories
# FMT_LIBRARIES - libraries needed to use fmt

find_path(FMT_INCLUDE_DIR
    NAMES fmt/format.h
    PATH_SUFFIXES include
)

find_library(FMT_LIBRARY
    NAMES fmt fmtd
    PATH_SUFFIXES lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Fmt
    REQUIRED_VARS FMT_LIBRARY FMT_INCLUDE_DIR
)

if(FMT_FOUND)
    set(FMT_LIBRARIES ${FMT_LIBRARY})
    set(FMT_INCLUDE_DIRS ${FMT_INCLUDE_DIR})

    if(NOT TARGET fmt::fmt)
        add_library(fmt::fmt UNKNOWN IMPORTED)
        set_target_properties(fmt::fmt PROPERTIES
            IMPORTED_LOCATION "${FMT_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${FMT_INCLUDE_DIR}"
        )
    endif()
endif() 