cmake_minimum_required(VERSION 3.20)
project(qevoryx_tui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
  GIT_TAG v7.0.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(ftxui)

add_executable(qevoryx_tui src/main.cpp)
target_link_libraries(qevoryx_tui
  PRIVATE ftxui::screen ftxui::dom ftxui::component)
