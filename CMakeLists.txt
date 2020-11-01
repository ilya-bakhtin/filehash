cmake_minimum_required(VERSION 2.8)

project(filehash)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} \
    -Wall -Wextra -Werror -pedantic -pedantic-errors")

set(SOURCES ${PROJECT_SOURCE_DIR}/hash-calculator.cpp ${PROJECT_SOURCE_DIR}/workers.cpp ${PROJECT_SOURCE_DIR}/filehash.cpp)
set(HEADERS ${PROJECT_SOURCE_DIR}/hash-calculator.h   ${PROJECT_SOURCE_DIR}/workers.h)

add_executable(filehash ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME} OpenSSL::Crypto)
target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
