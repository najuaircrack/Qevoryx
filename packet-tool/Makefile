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
    src/protocol/checksum.cpp \
    src/random/fast_random.cpp \
    src/transport/file_transport.cpp \
    src/transport/test_transport.cpp \
    src/monitor/console_monitor.cpp \
    src/monitor/monitor_factory.cpp

OBJECTS := $(SOURCES:.cpp=.o)
TARGET := packet-tool

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
