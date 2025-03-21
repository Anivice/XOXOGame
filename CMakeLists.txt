cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(XOXOGame C CXX)

include_directories(src/include)

add_compile_definitions(__LOG_TO_STDOUT__)

add_library(log OBJECT
        src/log.cpp src/include/log.hpp
)

add_library(space_and_objects OBJECT
        src/space.cpp src/include/space.h
)
target_link_libraries(space_and_objects PRIVATE log)

add_executable(draft_log unit_drafts/draft_log.cpp)
target_link_libraries(draft_log log)

add_executable(draft_space unit_drafts/draft_space.cpp)
target_link_libraries(draft_space space_and_objects)
