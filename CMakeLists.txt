cmake_minimum_required(VERSION 3.20)
project(qevoryx LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Core sources (shared by all targets) ──
set(QEVORYX_CORE_SOURCES
  src/app/application.cpp
  src/app/application_controller.cpp
  src/app/thread_affinity.cpp
  src/common/network_interfaces.cpp
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

# ── C2 switch ──────────────────────────────────────────────
# The C2 server/agent is NOT open source and lives in the private repo
# (branch `c2`). A public clone has no src/c2, src/server, src/agent,
# include/c2, include/server, include/agent directories — in that case the
# build automatically degrades to a local-only binary.
option(QEVORYX_ENABLE_C2 "Build C2 server/agent (requires private C2 sources)" ON)
if(QEVORYX_ENABLE_C2 AND NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/c2/channel.cpp")
  message(WARNING "C2 sources not present - building local-only binary (QEVORYX_ENABLE_C2=OFF)")
  set(QEVORYX_ENABLE_C2 OFF)
endif()

# ── C2 sources (shared by server and agent, private) ──
set(QEVORYX_C2_SOURCES
  src/c2/channel.cpp
  src/server/c2_config.cpp
  src/server/audit_log.cpp
  src/server/loot_store.cpp
  src/server/health_endpoint.cpp
  src/server/server_persist.cpp
)

add_library(qevoryx_core STATIC ${QEVORYX_CORE_SOURCES})
target_include_directories(qevoryx_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

find_package(Threads REQUIRED)
target_link_libraries(qevoryx_core PUBLIC Threads::Threads)

if(QEVORYX_ENABLE_C2)
  add_library(qevoryx_c2 STATIC ${QEVORYX_C2_SOURCES})
  target_include_directories(qevoryx_c2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_compile_definitions(qevoryx_core PUBLIC QEVORYX_ENABLE_C2)
  target_link_libraries(qevoryx_c2 PUBLIC qevoryx_core)
endif()

if(WIN32)
  target_link_libraries(qevoryx_core PUBLIC ws2_32 iphlpapi bcrypt)
  if(QEVORYX_ENABLE_C2)
    target_link_libraries(qevoryx_c2 PUBLIC ws2_32 bcrypt)
  endif()
elseif(UNIX)
  target_link_libraries(qevoryx_core PUBLIC pthread)
endif()

# ── Main Qevoryx binary (with FTXUI) ──
include(FetchContent)
FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
  GIT_TAG v7.0.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(ftxui)

set(QEVORYX_EXE_SOURCES
  src/main.cpp
  src/ftxui/application.cpp
  src/ftxui/view.cpp
)
if(QEVORYX_ENABLE_C2)
  list(APPEND QEVORYX_EXE_SOURCES
    src/server/server.cpp
    src/server/serve.cpp
  )
endif()

add_executable(qevoryx ${QEVORYX_EXE_SOURCES})
target_include_directories(qevoryx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/ftxui)
if(QEVORYX_ENABLE_C2)
  target_compile_definitions(qevoryx PRIVATE QEVORYX_ENABLE_C2)
  target_link_libraries(qevoryx PRIVATE qevoryx_core qevoryx_c2 ftxui::screen ftxui::dom ftxui::component)
else()
  target_link_libraries(qevoryx PRIVATE qevoryx_core ftxui::screen ftxui::dom ftxui::component)
endif()
if(WIN32)
  if(MSVC)
    target_compile_options(qevoryx PRIVATE /bigobj)
  endif()
  target_link_libraries(qevoryx PRIVATE ws2_32 iphlpapi bcrypt)
endif()

# ── C2 Agent binary (private sources only) ──
if(QEVORYX_ENABLE_C2)
  add_executable(qevoryx-agent
    src/agent/main.cpp
    src/agent/agent.cpp
    src/agent/persistence.cpp
  )
  target_include_directories(qevoryx-agent PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(qevoryx-agent PRIVATE qevoryx_c2 Threads::Threads)
  if(WIN32)
    target_link_libraries(qevoryx-agent PRIVATE ws2_32 iphlpapi bcrypt)
  endif()
endif()

# ── Install targets ──
if(QEVORYX_ENABLE_C2)
  install(TARGETS qevoryx qevoryx-agent
    RUNTIME DESTINATION bin
  )
else()
  install(TARGETS qevoryx
    RUNTIME DESTINATION bin
  )
endif()
