
# stdx

STDX is a minimalist, modular C99 utility library inspired by the STB style. It provides a set of dependency-free, single-header components that extend common programming functionality in C, aiming to become the kind of standard, lightweight utility library I wish the language had by default.


## Features Overview

STDX ships as a set of modules inside `src/`, each focused, readable, and self-contained.

### Core

- `stdx_common` — Portability macros, compiler/OS detection, assertions, bit utilities, typedefs, tagged result pointers.  
- `stdx_time` — Cross-platform timers, high-resolution time measurement, time arithmetic, sleeps.

### Memory & Containers

- `stdx_arena` — Bump allocator with chunk growth, mark/rewind, trimming, zero-fill helpers.  
- `stdx_array` — Generic dynamic array with automatic growth and stack-like push/pop.  
- `stdx_hashtable` — Generic, callback-driven hash map with open addressing and tombstones.

### Strings & Text Utilities

- `stdx_string` — UTF-8 utilities, slices, stack-allocated small strings, searching, trimming, conversions, comparisons.  
- `stdx_strbuilder` — Fast dynamic string builders for UTF-8 and wide strings.  
- `stdx_ini` — Minimal INI parser with string interning and flat arrays.  

### Platform & System Helpers

- `stdx_filesystem` — Path utilities, directory walking, file operations, metadata, symlinks, watchers.  
- `stdx_network` — Unified socket API for TCP/UDP, IPv4/IPv6, polling, DNS, multicast/broadcast.  
- `stdx_io` — Thin wrapper around `FILE*` for consistent I/O and whole-file read/write helpers.  
- `stdx_thread` — Portable threads, mutexes, condition variables, sleep/yield, and a simple thread pool.

### Diagnostics & Tooling

- `stdx_log` — Colorful, structured logging with configurable output and levels.  
- `stdx_test` — Micro test framework with timing, assertions, and crash reporting.

---

## Installation

STDX is a single-header style library.  
To use a module:

1. Include the header where you need it:

   ```
   #include "stdx_array.h"
   ```

2. In **exactly one** translation unit, define the implementation macro:

   ```
   #define X_IMPL_ARRAY
   #include "stdx_array.h"
   ```

There is no global build system and no external dependencies.

---
