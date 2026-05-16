cmake_minimum_required(VERSION 3.22)
project(SimpleTCPChat)

set(CMAKE_CXX_STANDARD 20)
set(TESTS ON)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREAD ON)
find_package(Boost 1.74.0 REQUIRED COMPONENTS system thread regex log)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})
endif(Boost_FOUND)

find_package(OpenSSL REQUIRED)

set(USED_LIBS ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_REGEX_LIBRARY} ${Boost_LOG_LIBRARY})

include_directories("header")

set(LIB_SOURCE
        "source/Client.cpp"
        "source/Server.cpp"
)

# Сервер
add_executable(${PROJECT_NAME} ${LIB_SOURCE} "source/main.cpp")
target_link_libraries(${PROJECT_NAME} -lboost_log_setup ${USED_LIBS} OpenSSL::SSL OpenSSL::Crypto)

# Клиент
add_executable(${PROJECT_NAME}Client ${LIB_SOURCE} "source/client_main.cpp")
target_link_libraries(${PROJECT_NAME}Client -lboost_log_setup ${USED_LIBS} OpenSSL::SSL OpenSSL::Crypto)

if(TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    add_library(${PROJECT_NAME}_lib ${LIB_SOURCE})
    include_directories("tests")
    target_link_libraries(${PROJECT_NAME}_lib PUBLIC -lboost_log_setup ${USED_LIBS} OpenSSL::SSL OpenSSL::Crypto)
    add_executable(${PROJECT_NAME}_tests "tests/main.cpp" "tests/functional_test.cpp")
    target_link_libraries(${PROJECT_NAME}_tests SimpleTCPChat_lib -lgtest -lpthread -lgmock)
endif()
