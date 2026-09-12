CXX := g++
CXXFLAGS := -std=c++17 -O3 -march=native -mtune=native -funroll-loops -flto -Wall -Wextra -pthread
INCLUDES := -Iinclude

SOURCES := \
    src/main.cpp \
    src/app/application.cpp \
    src/app/thread_affinity.cpp \
    src/config/cli_parser.cpp \
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
    src/tui/tui.cpp

OBJECTS := $(SOURCES:.cpp=.o)
OBJECTS_STATIC := $(SOURCES:.cpp=.static.o)
TARGET := qevoryx
TARGET_STATIC := qevoryx-static

# Detect OS
ifeq ($(OS),Windows_NT)
    TARGET := qevoryx.exe
    TARGET_STATIC := qevoryx-static.exe
    CXXFLAGS += -lws2_32
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        CXXFLAGS += -lpthread
    endif
endif

.PHONY: all static clean

all: $(TARGET)

static: $(TARGET_STATIC)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@

$(TARGET_STATIC): $(OBJECTS_STATIC)
	$(CXX) $(CXXFLAGS) -static $(OBJECTS_STATIC) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

%.static.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -DSNAPSHOT_BUILD -c $< -o $@

clean:
	rm -f $(OBJECTS) $(OBJECTS_STATIC) $(TARGET) $(TARGET_STATIC)
ifeq ($(OS),Windows_NT)
	del /Q /F $(OBJECTS) $(OBJECTS_STATIC) $(TARGET) $(TARGET_STATIC) 2>nul
endif
