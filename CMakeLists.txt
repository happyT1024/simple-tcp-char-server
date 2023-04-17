cmake_minimum_required(VERSION 3.23)
project(SimpleTCPChat)

set(CMAKE_CXX_STANDARD 20)

SET(Boost_USE_STATIC_LIBS OFF)
SET(Boost_USE_MULTITHREAD ON)
FIND_PACKAGE(Boost 1.74.0 REQUIRED COMPONENTS system thread regex log)
IF(Boost_FOUND)
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
    LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
ENDIF(Boost_FOUND)

SET(USED_LIBS ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_REGEX_LIBRARY} ${Boost_LOG_LIBRARY})

add_executable(${PROJECT_NAME} Sourse/main.cpp Sourse/Server.cpp Sourse/Server.h Sourse/Client.cpp Sourse/Client.h Sourse/init_log.h Sourse/signalHandler.h)

target_link_libraries(${PROJECT_NAME} -lboost_log_setup ${USED_LIBS} )
