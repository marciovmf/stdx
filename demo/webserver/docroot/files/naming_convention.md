# Developer Naming Convention Guide

This guide outlines the naming conventions we follow in our codebase to ensure consistency, readability, and maintainability. Please adhere to the following rules when adding new code or modifying existing code.

## 1. Macros and Constants
- **Style**: Use ALL CAPS with underscores.
- **Public Constants**: Prefix with `X_<MODULE>_`.
- **Examples**:
  - `X_IO_MAX_BUFFER_SIZE`
  - `X_FS_TRACKING_ID_PREFIX`

## 2. Struct Names and Typedefs
- **Style**: Use CamelCase.
- **Public Type Names**: Prefix public types with `X`.
- **Examples**:
  - `GtkWidget`
  - `TrackingOrder`
  - `XFsDirectory`

## 3. Struct Members
- **Style**: Use snake_case for member variables.
- **Examples**:
  ```
  struct xfs_directory {
      char name[256];
      int permissions;
  };
  ```

## 4. Functions Operating on Structs
- **Style**: Use snake_case. Prefix with x_<module>_ to indicate the module and functionality.
- **Examples**:
  ```
  x_fs_directory_create()
  x_fs_directory_delete()
  x_fs_path_normalize()
  x_thread_join()
  ```
## 5. General Functions
- **Style**: Use snake_case for all other functions that do not specifically operate on structs. Ensure they remain descriptive.
- **Examples**:
  ```
  x_fs_initialize()
  x_logging_set_level()
  x_config_load()
  ```

## 6. Static Local Functions and Variables
- **Style**: Use a prefix of s_ followed by x_ to denote static scope.
- **Examples**:
  ```
  static void s_x_internal_function();
  static int s_x_local_variable;
  ```
