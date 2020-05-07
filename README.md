Godot Pck Tool
==============

A standalone executable for unpacking and packing Godot .pck files.

Command line usage
------------------

TODO: write documentation on command line usage

Building
--------

These are instructions for building this on Fedora, including cross compiling to Windows.

### Required libraries

```sh
sudo dnf install cmake gcc-c++ libstdc++-static mingw32-gcc-c++ mingw32-winpthreads-static
```

Also don't forget to init git submodules.

Then just
```sh
make
```

Also if you want to make a folder with the executables and cross compile:

```sh
make all-install
```
