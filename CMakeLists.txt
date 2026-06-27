cmake_minimum_required(VERSION 3.41)

project("CelEngine"
        VERSION 0.1.0)

if (PROJECT_IS_TOP_LEVEL)
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
endif ()

option(CEL_BUILD_TESTS "Build the test suite" ${PROJECT_IS_TOP_LEVEL})
option(CEL_BUILD_EXAMPLES "Build the examples" ${PROJECT_IS_TOP_LEVEL})

add_subdirectory(src)

if (CEL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()

if (CEL_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()