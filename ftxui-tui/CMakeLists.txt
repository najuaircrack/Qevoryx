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

set(QEVORYX_BACKEND_SOURCES
  ../src/app/application.cpp
  ../src/app/application_controller.cpp
  ../src/app/thread_affinity.cpp
  ../src/config/settings_store.cpp
  ../src/monitor/console_monitor.cpp
  ../src/monitor/monitor_factory.cpp
  ../src/packet/ack_strategy.cpp
  ../src/packet/icmp_strategy.cpp
  ../src/packet/packet.cpp
  ../src/packet/rst_strategy.cpp
  ../src/packet/synack_strategy.cpp
  ../src/packet/tcp_syn_strategy.cpp
  ../src/packet/udp_strategy.cpp
  ../src/protocol/checksum.cpp
  ../src/random/fast_random.cpp
  ../src/transport/file_transport.cpp
  ../src/transport/test_transport.cpp
)

add_executable(qevoryx_tui
  src/main.cpp
  ${QEVORYX_BACKEND_SOURCES}
)

target_include_directories(qevoryx_tui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
find_package(Threads REQUIRED)
target_link_libraries(qevoryx_tui
  PRIVATE ftxui::screen ftxui::dom ftxui::component Threads::Threads
)
if(WIN32)
  target_link_libraries(qevoryx_tui PRIVATE ws2_32 iphlpapi)
elseif(UNIX)
  target_link_libraries(qevoryx_tui PRIVATE pthread)
endif()
