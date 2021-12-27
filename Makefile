all: compile

current_dir = $(shell pwd)

# Uses podman build for the Linux version to make it more widely compatible
all-install: install-podman install-local-cross

build:
	mkdir build

# This just creates the install folder, see the other install-* targets for actually building
install:
	mkdir install

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

cmake-cross-debug: build-cross
	cd build-cross && mingw32-cmake .. -DCMAKE_BUILD_TYPE=Debug

compile-cross: cmake-cross
	$(MAKE) -C build-cross

install-local-cross: build-cross
	cd build-cross && mingw32-cmake .. -DCMAKE_INSTALL_PREFIX=../install
	$(MAKE) -C build-cross install

.PHONY: all-cross cmake-cross compile-cross install-local-cross

# Podman build
build-podman:
	mkdir build-podman

podman-build-image:
	podman build . --tag godotpcktool-builder --target builder

remove-podman-image:
	podman rmi godotpcktool-builder || true

# podman cache should be fast enough in most cases here so we always build the image
compile-podman: build-podman podman-build-image
	podman run --rm --mount type=bind,source=$(current_dir),destination=/src,relabel=shared,ro=true \
		--mount type=bind,source=$(current_dir)/build-podman,destination=/out,relabel=shared \
		godotpcktool-builder /src/in_container_build.sh

install-podman: compile-podman install
	cp -r build-podman/* install/

.PHONY: podman-build-image remove-podman-image compile-podman install-podman

# Utilities
clean: remove-podman-image
	rm -rf build build-cross build-podman install

.PHONY: clean
