Godot Pck Tool
==============

A standalone executable for unpacking and packing Godot .pck files.

Command line usage
------------------

For these you just need the GodotPckTool executable. Available from the releases page.

Note: if you don't install it on Linux you need to either use the full
path or put it in a folder and run it as `./godotpcktool` similarly to
Windows.

You can view the tool help by running `godotpcktool -h`

### Listing contents

Lists the files inside a pck file.

```sh
godotpcktool Thrive.pck
```

Long form:

```sh
godotpcktool --pack Thrive.pck --action list
```

### Extracting contents

Extracts the contents of a pck file.

```sh
godotpcktool Thrive.pck -a e -o extracted
```

Long form:

```sh
godotpcktool --pack Thrive.pck --action extract --output extracted
```

### Adding content

Adds content to an existing pck or creates a new pck. When creating a
new pck you can specify which Godot version the pck file says it is
packed with using the flag `set-godot-version`.

```sh
godotpcktool Thrive.pck -a a extracted --remove-prefix extracted
```

Long form:

```sh
godotpcktool --pack Thrive.pck --action add --remove-prefix extracted --file extracted
```

### Filters

Filters can be used to only act on a subset of files in a pck file, or
from the filesystem.

#### Min size

Specify the minimum size under which files are excluded:

```sh
godotpcktool --min-size-filter 1000
```

This will exclude files with size 999 bytes and below.

### Max size

Specify the maximum size above which files are excluded:

```sh
godotpcktool --max-size-filter 1000
```

NOTE: if you use max size to compliment min size extraction, you
should subtract one from the size, otherwise you'll operate on the
same files twice.

However if you want to work on exactly some size files you can specify the same size twice:
```sh
godotpcktool --min-size-filter 1 --max-size-filter 1
```

### General info

In the long form multiple files may be included like this:
```sh
godotpcktool ... --file firstfile,secondfile
```

Make sure to use quoting if your files contain spaces, otherwise the
files will be interpreted as other options.

In the short form the files can just be listed after the other
commands. If your file begins with a `-` you can prevent it from being
interpreted as a parameter by adding `--` between the parameters and
the list of files.


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
