Godot Pck Tool
==============

A standalone executable for unpacking and packing Godot .pck files.

Command line usage
------------------

For these you just need the GodotPckTool executable. Available from
the releases page. Or see the end of this file for building
instructions.

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

The files are added with the specified paths on the command line, but
with the prefix removed. So for example if there was a file called
`extracted/example.png` and `extracted/subfolder/file.txt` and the
above command was used, those files would get added to the pck as
`res://example.png` and `res://subfolder/file.txt`.

After adding files it is recommended to use the listing command to
view the resulting data inside the pck to verify the expected actions
happened correctly. When a new file matches exactly the path name
inside the pck, it will replace that file.

To have more control over the resulting paths inside the pck, see the
section below on the JSON commands.

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

However, if you want to work on exactly some size files you can specify the same size twice:
```sh
godotpcktool --min-size-filter 1 --max-size-filter 1
```

#### Include by name

The option to include files can be given a list of regular expressions that select only files
that match at least one of them to be processed. For example, you can list all files containing
"po" in their names with:
```sh
godotpcktool --include-regex-filter po
```

Or if you want to require that to be the file extension (note that different shells require
different escaping):
```sh
godotpcktool -i '\.po'
```

Multiple regular expressions can be separated by comma, or specified by giving the option
multiple times:
```sh
godotpcktool -i '\.po,\.txt'
godotpcktool -i '\.po' -i '\.txt'
```

If no include filter is specified, all files pass through it. So not specifying an include
filter means "process all files".

Note that filtering is case-sensitive.

#### Exclude by name

Files can also be excluded if they match a regular expression:
```sh
godotpcktool --exclude-regex-filter txt
```

If both include and exclude filters are specified, then first the include filter is applied,
after that the exclude filter is used to filter out files that passed the first filter.
For example to find files containing "po" but no "zh":
```sh
godotpcktool -i '\.po' -e 'zh'
```

#### Overriding filters

If you need more complex filtering you can specify regular expressions with
`--include-override-filter` which makes any file matching any of those
regular expression be included in the operation, even if another filter
would cause the file to be excluded. For example, you can use this to set
file size limits and then override those for specific type:
```sh
godotpcktool --min-size-filter 1000 --include-override-filter '\.txt'
```

#### JSON bulk operations

To have more control over the resulting paths inside the pck file,
there is a JSON operation API provided.

To use it, you first need to create a JSON file (`commands.json` in
the example command but any name can be used) with the following
structure (as many files can be specified as required):
```json
[
    {
        "file": "/path/to/file",
        "target": "overridden/path/file"
    },
    {
        "file": "LICENSE",
        "target": "example/path/LICENSE"
    }
]
```

Then run the following command to use it (the JSON command can always
be specified when the add operation is used):
```sh
godotpcktool Thrive.pck -a a --command-file commands.json
```

This will read `/path/to/file` which can be an absolute or a relative
path, and save it within the pck as `res://overridden/path/file` and
also the `LICENSE` file as `res://example/path/LICENSE`. This way it
is possible to use absolute paths and specify whatever path the file
should end up as in the pck file for maximum control.

Note that the full path without the `res://` prefix needs to be in the
JSON `target` property for where the file inside the pck should end up
in; this mode doesn't support specifying just the folder so multiple
files with `target` being `pck/folder` will overwrite each other
rather than being placed inside `pck/folder`. So always specify full
paths like `pck/folder/README.txt` rather than `pck/folder/` when
specifying the JSON commands.


### Advanced Options

#### Specifying Engine Version

When creating a .pck file it is possible to specify the Godot engine
version the .pck says it is created with:

```sh
godotpcktool NewPack.pck -a a some_file.txt --set-godot-version 3.5.0
```

Note that this approach **does not** override the engine version number in existing .pck
files. This currently only applies to new .pck files.

#### Scripting

It is possible to use the JSON bulk API without creating a temporary file. This is done by specifying `-` as the file to add and then writing the JSON to the tool's stdin and then closing it.

```sh
godotpcktool NewPack.pck -a a -
```

When starting the above command, the tool will read all lines from stdin until it is closed. At that point the input JSON is 
parsed. As stdin needs to be closed for the tool to continue this is not meant for interactive use, but only for scripting. See the above section about the JSON command files for a version usable on the command line.

See above for the format of the accepted JSON as it is the same as the JSON command file format.

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

These are instructions for building this on Fedora, including cross
compiling to Windows.

Note that native Linux build uses the glibc of the currently installed
system, which may be too new for older distros. For a build that
supports those, see the section about podman builds.

### Required libraries

```sh
sudo dnf install cmake gcc-c++ libstdc++-static mingw32-gcc-c++ mingw32-winpthreads-static
```

Also don't forget to init git submodules.

```sh
git submodule init
git submodule update
```

Then just:
```sh
make
```

Also if you want to make a folder with the executables and cross compile:

```sh
make all-install
```

### Podman build

Podman can be used to build a Linux binary using the oldest supported
Ubuntu LTS. This ensures widest compatibility of the resulting binary.

First make sure podman and make are installed, then run the make
target:
```sh
make compile-podman
```

Due to the use of C++ 17 and non-ancient cmake version, the oldest
working Ubuntu LTS is currently 22.04 (as 20.04 has ended support).
