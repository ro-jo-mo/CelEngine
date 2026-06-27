cmake_minimum_required(VERSION 3.23)

find_package(GTest CONFIG REQUIRED)

add_executable(CelTests
    WorldTests.cpp
    QueryTests.cpp
    ResourceManagerTests.cpp
    HierarchyTests.cpp
    SchedulingTests.cpp
)
target_link_libraries(CelTests PRIVATE CelEngine GTest::gtest GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(CelTests)