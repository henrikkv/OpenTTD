find_package(CURL REQUIRED)
find_package(RapidJSON QUIET)

# If RapidJSON is not found via find_package, try to find it in a standard location
if(NOT RapidJSON_FOUND AND NOT RAPIDJSON_FOUND)
    find_path(RAPIDJSON_INCLUDE_DIRS
        NAMES rapidjson/rapidjson.h
        PATHS
            ${CMAKE_SOURCE_DIR}/libs/rapidjson/include
            /usr/include
            /usr/local/include
    )
    if(RAPIDJSON_INCLUDE_DIRS)
        set(RAPIDJSON_FOUND TRUE)
    endif()
endif()

if(NOT RAPIDJSON_FOUND AND NOT RapidJSON_FOUND)
    message(FATAL_ERROR "RapidJSON library not found. Please install it or provide its location.")
endif()

include_directories(${CURL_INCLUDE_DIRS})
if(RAPIDJSON_INCLUDE_DIRS)
    include_directories(${RAPIDJSON_INCLUDE_DIRS})
endif()

add_files(
    metal_api.cpp
    metal_api.h
)

target_link_libraries(openttd_lib ${CURL_LIBRARIES}) 