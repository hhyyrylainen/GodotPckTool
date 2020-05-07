Standalone Godot Pck Tool
=========================

A standalone executable for unpacking and packing Godot .pck files.

Required libraries
------------------

```sh
sudo dnf install cmake gcc-c++ libstdc++-static mingw32-gcc-c++ mingw32-winpthreads-static
```

Then just
```sh
make
```

Also if you want to make a folder with the executables and cross compile:

```sh
make all-install
```
