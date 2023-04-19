cmake_minimum_required(VERSION 3.22)
project(${PROJECT_NAME}_bin)

set(CMAKE_CXX_STANDARD 20)

SET(Boost_USE_STATIC_LIBS OFF)
SET(Boost_USE_MULTITHREAD ON)
FIND_PACKAGE(Boost 1.74.0 REQUIRED COMPONENTS system thread regex log)
IF(Boost_FOUND)
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
    LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
ENDIF(Boost_FOUND)

SET(USED_LIBS ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_REGEX_LIBRARY} ${Boost_LOG_LIBRARY})

add_executable(${PROJECT_NAME} main.cpp Server.cpp Server.h Client.cpp Client.h init_log.h signalHandler.h)
add_library(SimpleTCPChat_lib main.cpp Server.cpp Server.h Client.cpp Client.h init_log.h signalHandler.h)

target_link_libraries(${PROJECT_NAME} -lboost_log_setup ${USED_LIBS} )
target_link_libraries(SimpleTCPChat_lib -lboost_log_setup ${USED_LIBS} )
