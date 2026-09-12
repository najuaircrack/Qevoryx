CXX := g++
CXXFLAGS := -std=c++17 -O3 -march=x86-64 -mtune=generic -funroll-loops -flto -Wall -Wextra -pthread -MMD -MP
INCLUDES := -Iinclude
LDLIBS :=

SOURCES := \
    src/main.cpp \
    src/app/application.cpp \
    src/app/application_controller.cpp \
    src/app/thread_affinity.cpp \
    src/config/cli_parser.cpp \
    src/config/settings_store.cpp \
    src/packet/packet.cpp \
    src/packet/tcp_syn_strategy.cpp \
    src/packet/udp_strategy.cpp \
    src/packet/icmp_strategy.cpp \
    src/packet/ack_strategy.cpp \
    src/packet/rst_strategy.cpp \
    src/packet/synack_strategy.cpp \
    src/protocol/checksum.cpp \
    src/random/fast_random.cpp \
    src/transport/file_transport.cpp \
    src/transport/test_transport.cpp \
    src/monitor/console_monitor.cpp \
    src/monitor/monitor_factory.cpp \
    src/ui/tui_application.cpp \
    src/ui/tui_state.cpp \
    src/ui/tui_theme.cpp \
    src/ui/tui_layout.cpp \
    src/ui/renderer.cpp \
    src/ui/widgets/panel.cpp \
    src/ui/widgets/header.cpp \
    src/ui/widgets/configuration_panel.cpp \
    src/ui/widgets/actions_panel.cpp \
    src/ui/widgets/status_panel.cpp \
    src/ui/widgets/event_log_panel.cpp \
    src/ui/widgets/footer.cpp \
    src/ui/widgets/modal.cpp \
    src/ui/widgets/selectable_list.cpp \
    src/ui/widgets/value_editor.cpp \
    src/ui/widgets/help_panel.cpp \
    src/ui/widgets/runtime_panel.cpp \
    src/ui/widgets/live_status_panel.cpp

OBJECTS := $(SOURCES:.cpp=.o)
OBJECTS_STATIC := $(SOURCES:.cpp=.static.o)
TARGET := qevoryx
TARGET_STATIC := qevoryx-static

# Detect OS
UNAME_S := $(shell uname -s 2>/dev/null || echo Unknown)

ifneq (,$(findstring MINGW,$(UNAME_S)))
    TARGET := qevoryx.exe
    TARGET_STATIC := qevoryx-static.exe
    LDLIBS += -lws2_32 -liphlpapi -lpanelw -lncursesw
endif
ifneq (,$(findstring MSYS,$(UNAME_S)))
    TARGET := qevoryx.exe
    TARGET_STATIC := qevoryx-static.exe
    LDLIBS += -lws2_32 -liphlpapi -lpanelw -lncursesw
endif
ifeq ($(UNAME_S),Linux)
    NCURSES_LIBS := $(shell pkg-config --static --libs panelw ncursesw 2>/dev/null || printf '%s' '-lpanelw -lncursesw -ltinfo')
    LDLIBS += $(NCURSES_LIBS)
endif

.PHONY: all static clean

all: $(TARGET)

static: $(TARGET_STATIC)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@ $(LDLIBS)

$(TARGET_STATIC): $(OBJECTS_STATIC)
	$(CXX) $(CXXFLAGS) -static $(OBJECTS_STATIC) -o $@ $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

%.static.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -DSNAPSHOT_BUILD -c $< -o $@

-include $(OBJECTS:.o=.d) $(OBJECTS_STATIC:.o=.d)

clean:
	rm -f $(OBJECTS) $(OBJECTS_STATIC) $(TARGET) $(TARGET_STATIC)
