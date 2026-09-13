cmake_minimum_required(VERSION 3.20)
project(qevoryx LANGUAGES CXX)

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

set(QEVORYX_CORE_SOURCES
  src/app/application.cpp
  src/app/application_controller.cpp
  src/app/thread_affinity.cpp
  src/config/cli_parser.cpp
  src/config/settings_store.cpp
  src/monitor/console_monitor.cpp
  src/monitor/monitor_factory.cpp
  src/packet/ack_strategy.cpp
  src/packet/icmp_strategy.cpp
  src/packet/packet.cpp
  src/packet/rst_strategy.cpp
  src/packet/synack_strategy.cpp
  src/packet/tcp_syn_strategy.cpp
  src/packet/udp_strategy.cpp
  src/protocol/checksum.cpp
  src/random/fast_random.cpp
  src/transport/file_transport.cpp
  src/transport/test_transport.cpp
)

add_library(qevoryx_core STATIC ${QEVORYX_CORE_SOURCES})
target_include_directories(qevoryx_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
find_package(Threads REQUIRED)
target_link_libraries(qevoryx_core PUBLIC Threads::Threads)
if(WIN32)
  target_link_libraries(qevoryx_core PUBLIC ws2_32 iphlpapi)
elseif(UNIX)
  target_link_libraries(qevoryx_core PUBLIC pthread)
endif()

add_executable(qevoryx
  src/main.cpp
  src/ftxui/application.cpp
  src/ftxui/view.cpp
)
target_include_directories(qevoryx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/ftxui)
target_link_libraries(qevoryx PRIVATE qevoryx_core ftxui::screen ftxui::dom ftxui::component)
if(MSVC)
  target_compile_options(qevoryx PRIVATE /bigobj)
endif()
