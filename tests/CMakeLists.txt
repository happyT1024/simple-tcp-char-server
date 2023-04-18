cmake_minimum_required(VERSION 3.22)
project(tests)

enable_testing()

find_package(GTest REQUIRED)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} SimpleTCPChat_lib -lgtest -lpthread -lgmock)

gtest_discover_tests(${PROJECT_NAME})