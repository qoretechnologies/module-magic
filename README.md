# Qore Magic Module

A Qore language module providing a wrapper around libmagic for file type
detection using magic numbers. This module allows Qore programs to determine
the MIME type and encoding of files and data buffers without relying on
file extensions.

## Features

- Identify file types by analyzing file contents (magic bytes)
- Return MIME types and encoding information
- Support both file path analysis and in-memory buffer analysis
- Thread-safe operations
- Various flag options for customizing detection behavior

## Quick Example

```qore
%requires magic

# Get MIME type of a file
Magic m(MAGIC_MIME_TYPE);
printf("%s\n", m.file("/etc/hosts"));  # Output: text/plain

# Get MIME type with encoding
Magic m2(MAGIC_MIME);
printf("%s\n", m2.file("/etc/hosts"));  # Output: text/plain; charset=us-ascii

# Analyze in-memory data
printf("%s\n", m.buffer("Hello, World!"));  # Output: text/plain
```

## API

### Constructors

- `Magic()` - Create with default flags (MAGIC_NONE)
- `Magic(int flags)` - Create with specified flags

### Methods

- `string file(string fileName)` - Get magic info for a file
- `string file(string fileName, int flags)` - Get magic info with custom flags
- `string buffer(data data)` - Get magic info for a data buffer
- `string buffer(data data, int flags)` - Get magic info for buffer with custom flags
- `setFlags(int flags)` - Update the flags
- `int getFlags()` - Get current flags

### Common Flags

| Flag | Description |
|------|-------------|
| `MAGIC_NONE` | No special handling |
| `MAGIC_MIME_TYPE` | Return MIME type only |
| `MAGIC_MIME_ENCODING` | Return MIME encoding only |
| `MAGIC_MIME` | Return MIME type with encoding |
| `MAGIC_COMPRESS` | Check inside compressed files |
| `MAGIC_SYMLINK` | Follow symbolic links |
| `MAGIC_ERROR` | Treat errors as fatal |

## Build Requirements

- Qore development environment (lib and headers) >= 0.9
- CMake >= 2.8.12
- libmagic development files (file-devel package)
- C++11 compatible compiler
- (optional) Doxygen for documentation

## Build Instructions

Use an "out of source" build:

```bash
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/path/to/install ..
make
make install
```

If `CMAKE_INSTALL_PREFIX` is not specified, the Qore module directory is used.

## Running Tests

```bash
cd build
qore -l ./magic-api-*.qmod ../test/magic.qtest
```

## License

LGPL 2.1 - see [COPYING](COPYING) for details.
