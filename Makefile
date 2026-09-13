.PHONY: all clean configure

all: configure
	cmake --build build --config Release

configure:
	cmake -S . -B build

clean:
	rm -rf build
