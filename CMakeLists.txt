cmake_minimum_required(VERSION 3.30)

set(CMAKE_DEBUG_TYPE Debug)

project(starfetch C)

find_package(Curses)

include_directories(${CURSES_INCLUDE_DIRS})
add_executable(starfetch main.c)
target_link_libraries(starfetch ${CURSES_LIBRARIES})
target_link_libraries(starfetch m)

add_custom_command(
  TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND ${PROJECT_NAME}
  COMMENT "Running Executable")
