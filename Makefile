all: compile
# test

all-install: install-local install-local-cross

build:
	mkdir build

cmake: build
	cd build && cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake-debug: build
	cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug

compile: cmake
	$(MAKE) -C build

install-local: build
	cd build && cmake .. -DCMAKE_INSTALL_PREFIX=../install
	$(MAKE) -C build install

# Not implemented yet
test: cmake
	$(MAKE) -C build test

.PHONY: all cmake compile test all-install install-local

# Cross compile to windows
all-cross: compile-cross

build-cross:
	mkdir build-cross

cmake-cross: build-cross
	cd build-cross && mingw32-cmake ..

compile-cross: cmake-cross
	$(MAKE) -C build-cross

install-local-cross: build-cross
	cd build-cross && mingw32-cmake .. -DCMAKE_INSTALL_PREFIX=../install
	$(MAKE) -C build-cross install

.PHONY: all-cross cmake-cross compile-cross install-local-cross

clean:
	rm -rf build build-cross install

.PHONY: clean
