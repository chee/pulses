cmake_minimum_required(VERSION 3.2)

project(pulses)

include_directories(${PROJECT_SOURCE_DIR})
add_executable(pulses pulses.c)
target_link_libraries(pulses jack sndfile)
 