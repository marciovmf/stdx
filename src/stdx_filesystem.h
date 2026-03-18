/**
 * STDX - Filesystem Utilities
 * Part of the STDX General Purpose C Library by marciovmf
 * License: MIT
 * <https://github.com/marciovmf/stdx>
 *
 * ## Overview
 *
 *  Provides a cross-platform filesystem abstraction including:
 *
 * - Directory and file operations (create, delete, copy, rename, enumerate)
 * - Filesystem monitoring via watch APIs
 * - Rich path manipulation utilities (normalize, join, basename, dirname, extension, relative paths)
 * - File metadata retrieval and modification (timestamps, permissions)
 * - Utilities for symbolic links and file type queries
 * - Temporary file and directory creation
 * - Functions accepts and preserves valid UTF-8 paths.
 *
 * ## How to compile
 * To compile the implementation define `X_IMPL_FILESYSTEM` 
 * in **one** source file before including this header.
 *
 * To customize how this module allocates memory, define
 * `X_FILESYSTEM_ALLOC` / `X_FILESYSTEM_CALLOC` / `X_FILESYSTEM_FREE` before including.
 *
 *
 * ## Dependencies
 *  stdx_string.h
 */

#ifndef X_FILESYSTEM_H
#define X_FILESYSTEM_H

#ifdef X_IMPL_FILESYSTEM
#ifndef X_IMPL_STRING
#define X_INTERNAL_STRING_IMPL
#define X_IMPL_STRING
#endif
#endif
#include "stdx_string.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifndef X_FILESYSTEM_API
#define X_FILESYSTEM_API
#endif

#define X_FILESYSTEM_VERSION_MAJOR 1
#define X_FILESYSTEM_VERSION_MINOR 0
#define X_FILESYSTEM_VERSION_PATCH 0
#define X_FILESYSTEM_VERSION (X_FILESYSTEM_VERSION_MAJOR * 10000 + X_FILESYSTEM_VERSION_MINOR * 100 + X_FILESYSTEM_VERSION_PATCH)

#ifndef X_FS_PAHT_MAX_LENGTH
# define X_FS_PAHT_MAX_LENGTH 512
#endif

#ifdef _WIN32
#define X_FS_PATH_SEPARATOR '\\'
#define X_FS_ALT_PATH_SEPARATOR '/'
#else
#define X_FS_PATH_SEPARATOR '/'
#define X_FS_ALT_PATH_SEPARATOR '\\'
#endif

#ifdef __cplusplus
extern "C" {
#endif

  typedef XSmallstr XFSPath;
  typedef struct XFSDireEntry_t XFSDireEntry;
  typedef struct XFSDireHandle_t XFSDireHandle;
  typedef struct XFSWatch_t XFSWatch;

  struct XFSDireEntry_t
  {
    char name[X_FS_PAHT_MAX_LENGTH]; 
    size_t size;
    time_t last_modified;
    int32_t is_directory;
  }; 

  typedef enum
  {
    x_fs_watch_CREATED,
    x_fs_watch_DELETED,
    x_fs_watch_MODIFIED,
    x_fs_watch_RENAMED_FROM,
    x_fs_watch_RENAMED_TO,
    x_fs_watch_UNKNOWN
  } XFSWatchEventType;

  typedef struct
  {
    XFSWatchEventType action;
    const char* filename; // Valid until next poll
  } XFSWatchEvent;

  typedef struct XFSTime
  {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
  } XFSTime;

  // Filesystem operations
  // Path manipulation
#define       x_fs_path(out, ...) x_fs_path_(out, __VA_ARGS__, 0)
#define       x_fs_path_join(path, ...) x_fs_path_join_(path, __VA_ARGS__, 0)
#define       x_fs_path_join_slice(path, ...) x_fs_path_join_slice_(path, __VA_ARGS__, 0)

  // Path manipulation
  typedef struct
  {
    size_t size;
    time_t creation_time;
    time_t modification_time;
    uint32_t permissions; // Platform dependent
  } FSFileStat;

  /**
   * @brief Begin a directory enumeration and retrieve the first entry.
   * @param path Directory path to search.
   * @param entry Output directory entry filled with the first result.
   * @return Handle used for subsequent enumeration, or NULL on failure.
   */
  X_FILESYSTEM_API XFSDireHandle* x_fs_find_first_file(const char* path, XFSDireEntry* entry);

  /**
   * @brief Create a directory.
   * @param path Directory path to create.
   * @return True on success, false on failure.
   */
  X_FILESYSTEM_API bool x_fs_directory_create(const char* path);

  /**
   * @brief Create a directory and any missing parent directories.
   * @param path Directory path to create.
   * @return True on success, false on failure.
   */
  X_FILESYSTEM_API bool x_fs_directory_create_recursive(const char* path);

  /**
   * @brief Delete an empty directory.
   * @param directory Directory path to delete.
   * @return True on success, false on failure.
   */
  X_FILESYSTEM_API bool x_fs_directory_delete(const char* directory);

  /**
   * @brief Copy a file to a new path.
   * @param file Source file path.
   * @param newFile Destination file path.
   * @return True on success, false on failure.
   */
  X_FILESYSTEM_API bool x_fs_file_copy(const char* file, const char* newFile);

  /**
   * @brief Rename (move) a file to a new path.
   * @param file Source file path.
   * @param newFile Destination file path.
   * @return True on success, false on failure.
   */
  X_FILESYSTEM_API bool x_fs_file_rename(const char* file, const char* newFile);

  /**
   * @brief Continue a directory enumeration and retrieve the next entry.
   * @param dirHandle Enumeration handle returned by x_fs_find_first_file().
   * @param entry Output directory entry filled with the next result.
   * @return True if an entry was written, false if no more entries or on failure.
   */
  X_FILESYSTEM_API bool x_fs_find_next_file(XFSDireHandle* dirHandle, XFSDireEntry* entry);

  /**
   * @brief Get the system temporary folder path.
   * @param out Output path buffer to receive the temp folder.
   * @return Length of the written path (in bytes/chars, excluding terminator), or 0 on failure.
   */
  X_FILESYSTEM_API size_t x_fs_get_temp_folder(XFSPath* out);

  /**
   * @brief Get the current working directory.
   * @param path Output path buffer to receive the current working directory.
   * @return Length of the written path (in bytes/chars, excluding terminator), or 0 on failure.
   */
  X_FILESYSTEM_API size_t x_fs_cwd_get(XFSPath* path);

  /**
   * @brief Set the current working directory.
   * @param path New working directory path.
   * @return Non-zero on success, 0 on failure.
   */
  X_FILESYSTEM_API size_t x_fs_cwd_set(const char* path);

  /**
   * @brief Set the current working directory to the executable's directory.
   * @return Non-zero on success, 0 on failure.
   */
  X_FILESYSTEM_API size_t x_fs_cwd_set_from_executable_path(void);

  /**
   * @brief Close a directory enumeration handle.
   * @param dirHandle Enumeration handle to close.
   * @return Nothing.
   */
  X_FILESYSTEM_API void x_fs_find_close(XFSDireHandle* dirHandle);

  /**
   * @brief Open a filesystem watcher for the given path.
   * @param path Path to watch (directory or file, depending on platform support).
   * @return Watch handle, or NULL on failure.
   */
  X_FILESYSTEM_API XFSWatch* x_fs_watch_open(const char* path);

  /**
   * @brief Close a filesystem watcher.
   * @param fw Watch handle to close.
   * @return Nothing.
   */
  X_FILESYSTEM_API void x_fs_watch_close(XFSWatch* fw);

  /**
   * @brief Poll a filesystem watcher for pending events.
   * @param fw Watch handle.
   * @param out_events Output array to receive events.
   * @param max_events Maximum number of events to write to out_events.
   * @return Number of events written, or a negative value on error.
   */
  X_FILESYSTEM_API int32_t x_fs_watch_poll(XFSWatch* fw, XFSWatchEvent* out_events, int32_t max_events);

  /**
   * @brief Normalize a path in-place (e.g., separators, ".", ".." where possible).
   * @param input Path to normalize.
   * @return Pointer to the normalized path (same as input).
   */
  X_FILESYSTEM_API XFSPath* x_fs_path_normalize(XFSPath* input);

  /**
   * @brief Get the filename stem (basename without extension) from a path string.
   * @param input Path as a C string.
   * @return Slice pointing into input containing the stem.
   */
  X_FILESYSTEM_API XSlice x_fs_path_stem_cstr(const char* input);
  X_FILESYSTEM_API XSlice x_fs_path_stem_as_slice(const XFSPath* input);
  X_FILESYSTEM_API size_t x_fs_path_stem(const XFSPath* input, XFSPath* out);


  /**
   * @brief Get the basename (final component) from a path string.
   * @param input Path as a C string.
   * @return Slice pointing into input containing the basename.
   */
  X_FILESYSTEM_API XSlice x_fs_path_basename_cstr(const char* input);
  X_FILESYSTEM_API XSlice x_fs_path_basename_as_slice(const XFSPath* input);
  X_FILESYSTEM_API size_t x_fs_path_basename(const XFSPath* input, XFSPath* out);


  /**
   * @brief Get the directory name (everything before the final component) from a path string.
   * @param input Path as a C string.
   * @return Slice pointing into input containing the directory portion.
   */
  X_FILESYSTEM_API XSlice x_fs_path_dirname_cstr(const char* input);
  X_FILESYSTEM_API XSlice x_fs_path_dirname_as_slice(const XFSPath* input);
  X_FILESYSTEM_API size_t x_fs_path_dirname(const XFSPath* input, XFSPath* out);


  /**
   * @brief Build a path from one or more C string components (variadic), writing to out.
   * @param out Output path to receive the resulting path.
   * @return True on success, false on failure.
   */
  X_FILESYSTEM_API bool x_fs_path_(XFSPath* out, ...);

  /* Join path segments (NULL-terminated varargs). Used by x_fs_path_join(). */
  X_FILESYSTEM_API size_t x_fs_path_join_(XFSPath* path, ...);

  /* Join path segments from XSlice pointers (NULL-terminated varargs). Used by x_fs_path_join_slice(). */
  X_FILESYSTEM_API size_t x_fs_path_join_slice_(XFSPath* path, ...);

  /* Full-path view as XSlice. Valid while the XFSPath is not modified. */
  X_FILESYSTEM_API XSlice x_fs_path_as_slice(const XFSPath* path);

  /**
   * @brief Check whether a path exists on disk.
   * @param path Path to check.
   * @return True if it exists, false otherwise.
   */
  X_FILESYSTEM_API bool x_fs_path_exists(const XFSPath* path);

  /**
   * @brief Check whether a C-string path exists on disk.
   * @param path Path to check.
   * @return True if it exists, false otherwise.
   */
  X_FILESYSTEM_API bool x_fs_path_exists_cstr(const char* path);

  /**
   * @brief Check whether a path exists on disk using a faster, less thorough method.
   * @param path Path to check.
   * @return True if it exists, false otherwise.
   */
  X_FILESYSTEM_API bool x_fs_path_exists_quick(const XFSPath* path);

  /**
   * @brief Check whether a C-string path exists on disk using a faster, less thorough method.
   * @param path Path to check.
   * @return True if it exists, false otherwise.
   */
  X_FILESYSTEM_API bool x_fs_path_exists_quick_cstr(const char* path);

  /**
   * @brief Check whether a path is absolute.
   * @param path Path to check.
   * @return True if absolute, false otherwise.
   */
  X_FILESYSTEM_API bool x_fs_path_is_absolute(const XFSPath* path);

  /**
   * @brief Check whether a C-string path is absolute.
   * @param path Path to check.
   * @return True if absolute, false otherwise.
   */
  X_FILESYSTEM_API bool x_fs_path_is_absolute_cstr(const char* path);

  /**
   * @brief Check whether a path is absolute using native platform rules.
   * @param path Path to check.
   * @return True if absolute, false otherwise.
   */
  X_FILESYSTEM_API bool x_fs_path_is_absolute_native(const XFSPath* path);

  /**
   * @brief Check whether a C-string path is absolute using native platform rules.
   * @param path Path to check.
   * @return True if absolute, false otherwise.
   */
  X_FILESYSTEM_API bool x_fs_path_is_absolute_native_cstr(const char* path);

  /**
   * @brief Check whether a path points to an existing directory.
   * @param path Path to check.
   * @return True if it is a directory, false otherwise.
   */
  X_FILESYSTEM_API bool x_fs_path_is_directory(const XFSPath* path);

  /**
   * @brief Check whether a C-string path points to an existing directory.
   * @param path Path to check.
   * @return True if it is a directory, false otherwise.
   */
  X_FILESYSTEM_API bool x_fs_path_is_directory_cstr(const char* path);

  /**
   * @brief Check whether a path points to an existing file.
   * @param path Path to check.
   * @return True if it is a file, false otherwise.
   */
  X_FILESYSTEM_API bool x_fs_path_is_file(const XFSPath* path);

  /**
   * @brief Check whether a C-string path points to an existing file.
   * @param path Path to check.
   * @return True if it is a file, false otherwise.
   */
  X_FILESYSTEM_API bool x_fs_path_is_file_cstr(const char* path);

  /**
   * @brief Check whether a path is relative.
   * @param path Path to check.
   * @return True if relative, false otherwise.
   */
  X_FILESYSTEM_API bool x_fs_path_is_relative(const XFSPath* path);

  /**
   * @brief Check whether a C-string path is relative.
   * @param path Path to check.
   * @return True if relative, false otherwise.
   */
  X_FILESYSTEM_API bool x_fs_path_is_relative_cstr(const char* path);

  /**
   * @brief Get a NUL-terminated C string view of an XFSPath.
   * @param p Path object.
   * @return Pointer to the internal NUL-terminated string.
   */
  X_FILESYSTEM_API const char* x_fs_path_cstr(const XFSPath* p);

  /**
   * @brief Append a single component to a path.
   * @param p Path to append to.
   * @param comp Component to append.
   * @return New length of the path after appending.
   */
  X_FILESYSTEM_API size_t x_fs_path_append(XFSPath* p, const char* comp);

  /**
   * @brief Replace or set the extension of a path.
   * @param path Path to modify.
   * @param new_ext New extension (with or without leading dot, depending on implementation).
   * @return New length of the path after the change.
   */
  X_FILESYSTEM_API size_t x_fs_path_change_extension(XFSPath* path, const char* new_ext);

  /**
   * @brief Compare two paths, ignoring separator type.
   * @param a First path.
   * @param b Second path.
   * @return 0 if equal, <0 if a < b, >0 if a > b.
   */
  X_FILESYSTEM_API size_t x_fs_path_compare(const XFSPath* a, const XFSPath* b);

  /**
   * @brief Compare a path against a C string path, ignoring separator type.
   * @param a Path.
   * @param cstr C string path.
   * @return 0 if equal, <0 if a < cstr, >0 if a > cstr.
   */
  X_FILESYSTEM_API int32_t x_fs_path_compare_cstr(const XFSPath* a, const char* cstr);

  /**
   * @brief Check whether a path equals a small string.
   * @param a Path.
   * @param b Small string to compare against.
   * @return True if equal, false otherwise.
   */
  X_FILESYSTEM_API bool x_fs_path_eq(const XFSPath* a, const XSmallstr* b);

  /**
   * @brief Check whether a path equals a C string path.
   * @param a Path.
   * @param b C string path.
   * @return True if equal, false otherwise.
   */
  X_FILESYSTEM_API bool x_fs_path_eq_cstr(const XFSPath* a, const char* b);

  /**
   * @brief Get the extension from a path string.
   * @param input Path as a C string.
   * @return Slice pointing into input containing the extension (may be empty).
   */
  X_FILESYSTEM_API XSlice x_fs_path_extension_cstr(const char* input);
  X_FILESYSTEM_API XSlice x_fs_path_extension_as_slice(const XFSPath* input);
  X_FILESYSTEM_API size_t x_fs_path_extension(const XFSPath* input, XFSPath* out);


  /**
   * @brief Convert a slice into an XFSPath.
   * @param sv Input slice containing a path string.
   * @param out Output path to receive the converted string.
   * @return Length written to out.
   */
  X_FILESYSTEM_API size_t x_fs_path_from_slice(XSlice sv, XFSPath* out);

  /**
   * @brief Append one or more path components to an existing path (variadic).
   * @param path Path to append into.
   * @return New length of the path after joining.
   */
  X_FILESYSTEM_API size_t x_fs_path_join_(XFSPath* path, ...);

  X_FILESYSTEM_API size_t x_fs_path_join_slice_(XFSPath* path, ...);

  /**
   * @brief Compute a relative path from one C-string path to another.
   * @param from_path Base path.
   * @param to_path Target path.
   * @param out_path Output path receiving the relative path.
   * @return Length of the written relative path, or 0 on failure.
   */
  X_FILESYSTEM_API size_t x_fs_path_relative_to_cstr(const char* from_path, const char* to_path, XFSPath* out_path);

  /**
   * @brief Compute the common prefix of two paths.
   * @param from_path First path.
   * @param to_path Second path.
   * @param out_path Output path receiving the common prefix.
   * @return True if a common prefix was written (may be empty depending on implementation), false on failure.
   */
  X_FILESYSTEM_API bool x_fs_path_common_prefix(const char* from_path, const char* to_path, XFSPath* out_path);

  /**
   * @brief Set a path from a C string.
   * @param p Path to set.
   * @param cstr Source C string.
   * @return Length written to p.
   */
  X_FILESYSTEM_API size_t x_fs_path_set(XFSPath* p, const char* cstr);

  /**
   * @brief Set a path from a stdx slice.
   * @param p Path to set.
   * @param slice stdx slice string.
   * @return Length written to p.
   */
  X_FILESYSTEM_API size_t x_fs_path_set_slice(XFSPath* p, XSlice slice);

  /**
   * @brief Split a path string into components.
   * @param input Path as a C string.
   * @param out_components Output array of components.
   * @param max_components Maximum number of components to write.
   * @param out_count Output count of components written.
   * @return True on success, false on failure.
   */
  X_FILESYSTEM_API bool x_fs_path_split(const char* input, XFSPath* out_components, size_t max_components, size_t* out_count);

  /**
   * @brief Get the executable path (or executable directory, depending on implementation).
   * @param out Output path to receive the executable path.
   * @return Length written to out, or 0 on failure.
   */
  X_FILESYSTEM_API size_t x_fs_path_from_executable(XFSPath* out);

  /**
   * @brief Retrieve metadata about a file or directory.
   * @param path File or directory path.
   * @param out_stat Output struct receiving the metadata.
   * @return True on success, false on failure.
   */
  X_FILESYSTEM_API bool x_fs_file_stat(const char* path, FSFileStat* out_stat);

  /**
   * @brief Retrieve the last modification time of a file or directory.
   * @param path File or directory path.
   * @param out_time Output time receiving the modification time.
   * @return True on success, false on failure.
   */
  X_FILESYSTEM_API bool x_fs_file_modification_time(const char* path, time_t* out_time);

  /**
   * @brief Retrieve the creation time of a file or directory.
   * @param path File or directory path.
   * @param out_time Output time receiving the creation time.
   * @return True on success, false on failure.
   */
  X_FILESYSTEM_API bool x_fs_file_creation_time(const char* path, time_t* out_time);

  /**
   * @brief Retrieve permissions metadata for a file or directory.
   * @param path File or directory path.
   * @param out_permissions Output value receiving the permissions (platform-dependent).
   * @return True on success, false on failure.
   */
  X_FILESYSTEM_API bool x_fs_file_permissions(const char* path, uint32_t* out_permissions);

  /**
   * @brief Set permissions metadata for a file or directory.
   * @param path File or directory path.
   * @param permissions Permissions value to set (platform-dependent).
   * @return True on success, false on failure.
   */
  X_FILESYSTEM_API bool x_fs_file_set_permissions(const char* path, uint32_t permissions);

  /**
   * @brief Check whether a path points to an existing regular file.
   * @param path Path to check.
   * @return True if it is a file, false otherwise.
   */
  X_FILESYSTEM_API bool x_fs_is_file(const char* path);

  /**
   * @brief Check whether a path points to an existing directory.
   * @param path Path to check.
   * @return True if it is a directory, false otherwise.
   */
  X_FILESYSTEM_API bool x_fs_is_directory(const char* path);

  /**
   * @brief Check whether a path is a symbolic link.
   * @param path Path to check.
   * @return True if it is a symlink, false otherwise.
   */
  X_FILESYSTEM_API bool x_fs_is_symlink(const char* path);

  /**
   * @brief Read the target of a symbolic link.
   * @param path Symbolic link path.
   * @param out_path Output path receiving the resolved link target.
   * @return True on success, false on failure.
   */
  X_FILESYSTEM_API bool x_fs_read_symlink(const char* path, XFSPath* out_path);

  /**
   * @brief Create a temporary file with the given prefix.
   * @param prefix Prefix to use for the temporary file name.
   * @param out_path Output path receiving the created temp file path.
   * @return True on success, false on failure.
   */
  X_FILESYSTEM_API bool x_fs_make_temp_file(const char* prefix, XFSPath* out_path);

  /**
   * @brief Create a temporary directory with the given prefix.
   * @param prefix Prefix to use for the temporary directory name.
   * @param out_path Output path receiving the created temp directory path.
   * @return True on success, false on failure.
   */
  X_FILESYSTEM_API bool x_fs_make_temp_directory(const char* prefix, XFSPath* out_path);


  /**
   * @brief Convert a time value from epoch to XFSTime.
   * @param t Time in seconds since Unix epoch.
   * @return XFSTime representation of the given epoch time.
   */
  X_FILESYSTEM_API XFSTime x_fs_time_from_epoch(time_t t);

  /**
   * @brief Compute a relative path from one path to another.
   * @param from_path Base path to compute the relative path from.
   * @param to_path Target path to compute the relative path to.
   * @param out_path Output path receiving the relative result.
   * @return Length of the resulting path on success, 0 on failure.
   *
   * @note The function assumes both paths are normalized.
   * @note The result may contain ".." segments if needed.
   */
  X_FILESYSTEM_API size_t x_fs_path_relative_to(const XFSPath* from_path, const XFSPath* to_path, XFSPath* out_path);



#ifdef __cplusplus
}
#endif


#ifdef X_IMPL_FILESYSTEM

#include <string.h>
#include <stdio.h>
#include <time.h>

#if defined(_WIN32) || defined(_WIN64)
#include <fileapi.h>
#include <windows.h>
#include <io.h>
#else
#include <sys/inotify.h>
#include <unistd.h>     // for readlink, access, etc.
#include <dirent.h>     // for DIR*, opendir, readdir
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>     // for PATH_MAX
#include <fcntl.h>

#ifndef MAX_PATH
#define MAX_PATH PATH_MAX
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h> // for _NSGetExecutablePath
#endif
#endif

#ifndef X_FS_MAX_PATH
#define X_FS_MAX_PATH MAX_PATH
#endif

#include <time.h>
#include <stdlib.h>

#ifndef X_FILESYSTEM_ALLOC
/**
 * @brief Internal macro for allocating memory.
 * To override how this header allocates memory, define this macro with a
 * different implementation before including this header.
 * @param sz  The size of memory to alloc.
 */
#define X_FILESYSTEM_ALLOC(sz)        malloc(sz)
#endif

#ifndef X_FILESYSTEM_CALLOC

/**
 * @brief Internal macro for allocating zeroed memory.
 * To override how this header allocates zeroed memory, define this macro with a
 * different implementation before including this header.
 * @param sz  The size of memory to alloc.
 */
#define X_FILESYSTEM_CALLOC(n,sz)     calloc((n),(sz))
#endif

#ifndef X_FILESYSTEM_FREE
/**
 * @brief Internal macro for freeing memory.
 * To override how this header frees memory, define this macro with a
 * different implementation before including this header.
 * @param p  The address of memory region to free.
 */
#define X_FILESYSTEM_FREE(p)          free(p)
#endif

#ifdef __cplusplus
extern "C" {
#endif

  struct XFSDireHandle_t
  {
#ifdef _WIN32
    HANDLE handle;
    WIN32_FIND_DATA findData;
#else
    DIR* dir;
    struct dirent* entry;
    struct stat fileStat;
    char base_path[X_FS_PAHT_MAX_LENGTH];
#endif
  }; 

  struct XFSWatch_t
  {
#ifdef _WIN32
    HANDLE dir;
    OVERLAPPED overlapped;
    char buffer[4096];
    DWORD last_bytes;
    int32_t ready;
#elif defined(__linux__)
    int32_t fd;
    int32_t wd;
    char buffer[4096];
    int32_t offset, len;
#endif
  };

#ifdef _WIN32
  X_FILESYSTEM_API static time_t s_x_fs_filetime_to_time_t_(const FILETIME* ft)
  {
    ULARGE_INTEGER ull;
    ull.LowPart = ft->dwLowDateTime;
    ull.HighPart = ft->dwHighDateTime;

    /* FILETIME is 100-ns intervals since 1601-01-01 (UTC). Convert to Unix epoch. */
    const uint64_t EPOCH_DIFF_100NS = 11644473600ULL * 10000000ULL;
    if (ull.QuadPart < EPOCH_DIFF_100NS)
      return (time_t)0;

    uint64_t t = (uint64_t)((ull.QuadPart - EPOCH_DIFF_100NS) / 10000000ULL);
    return (time_t)t;
  }
#endif

  X_FILESYSTEM_API int32_t x_fs_path_join_one_slice(XFSPath* out, const XSlice segment)
  {
    if (!out || !segment.ptr) return false;

    size_t seg_len = segment.length;
    if (seg_len == 0) return true;

    bool needs_sep = false;

    if (out->length > 0 && out->buf[out->length - 1] != X_FS_PATH_SEPARATOR)
    {
      needs_sep = true;
    }

    size_t total_needed = out->length + (needs_sep ? 1 : 0) + seg_len;

    // enought space ?
    if (total_needed > X_SMALLSTR_MAX_LENGTH) return false;

    // Add separator if needed
    if (needs_sep) out->buf[out->length++] = X_FS_PATH_SEPARATOR;

    // Append segment
    memcpy(&out->buf[out->length], segment.ptr, seg_len);
    out->length += seg_len;

    // Null-terminate
    out->buf[out->length] = '\0';
    return (int) out->length;
  }


  X_FILESYSTEM_API int32_t x_fs_path_join_one(XFSPath* out, const char* segment)
  {
    if (!out || !segment) return false;

    size_t seg_len = strlen(segment);
    if (seg_len == 0) return true; // Nothing to append

    bool needs_sep = false;

    if (out->length > 0 && out->buf[out->length - 1] != X_FS_PATH_SEPARATOR)
    {
      needs_sep = true;
    }

    size_t total_needed = out->length + (needs_sep ? 1 : 0) + seg_len;

    if (total_needed > X_SMALLSTR_MAX_LENGTH) return false; // Not enough space

    // Add separator if needed
    if (needs_sep) {
      out->buf[out->length++] = X_FS_PATH_SEPARATOR;
    }

    // Append segment
    memcpy(&out->buf[out->length], segment, seg_len);
    out->length += seg_len;

    // Null-terminate
    out->buf[out->length] = '\0';
    return (int) out->length;
  }

  X_FILESYSTEM_API bool x_fs_path_(XFSPath* out, ...)
  {
    va_list args;
    va_start(args, out);

    out->buf[0] = '\0';
    out->length = 0;

    const char* segment = NULL;
    while ((segment = va_arg(args, const char*)) != NULL)
    {
      size_t length = x_fs_path_join_one(out, segment);
      if (length >= X_FS_PAHT_MAX_LENGTH)
        return false;
    }

    va_end(args);
    return true;
  }

  X_FILESYSTEM_API size_t x_fs_cwd_get(XFSPath* path)
  {
    if (!path)
      return 0;
    path->length = 0;
#ifdef _WIN32
    DWORD size = GetCurrentDirectory(X_FS_PAHT_MAX_LENGTH, path->buf);
    path->length = size;
#else
    char* result = getcwd(path->buf, X_FS_PAHT_MAX_LENGTH);
    path->length = strlen(path->buf);
#endif
    return path->length;
  }

  X_FILESYSTEM_API size_t x_fs_cwd_set(const char* path)
  {
#ifdef _WIN32
    return SetCurrentDirectory(path) != 0;
#else
    return chdir(path) == 0;
#endif
  }

  X_FILESYSTEM_API size_t x_fs_path_from_executable(XFSPath* out)
  {
#ifdef _WIN32
    DWORD len = GetModuleFileNameA(NULL, out->buf, X_FS_PAHT_MAX_LENGTH);
    if (len == 0 || len >= X_FS_PAHT_MAX_LENGTH) return 0;
#elif defined(__APPLE__)
    uint32_t size = X_FS_PAHT_MAX_LENGTH;
    if (_NSGetExecutablePath(out->buf, &size) != 0) return 0;
    size_t len = strlen(out->buf);
#else
    ssize_t len = readlink("/proc/self/exe", out->buf, X_FS_PAHT_MAX_LENGTH - 1);
    if (len == -1) return 0;
    out->buf[len] = 0;
#endif

    out->length = len;
    return len;
  }

  X_FILESYSTEM_API size_t x_fs_cwd_set_from_executable_path(void)
  {
    XFSPath program_path, cwd;
    size_t bytesCopied = x_fs_path_from_executable(&program_path);
    XSlice dirname = x_fs_path_dirname_cstr((const char*) &program_path);
    x_fs_path_from_slice(dirname, &cwd);
    x_fs_cwd_set((const char*) &cwd);
    return bytesCopied;
  }

  X_FILESYSTEM_API bool x_fs_file_copy(const char* file, const char* newFile)
  {
#ifdef _WIN32
    return CopyFile(file, newFile, FALSE) != 0;
#else
    FILE *src = fopen(file, "rb");
    if (!src) return false;
    FILE *dst = fopen(newFile, "wb");
    if (!dst)
    { fclose(src); return false; }

    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), src)) > 0)
    {
      if (fwrite(buf, 1, n, dst) != n)
      {
        fclose(src); fclose(dst); return false;
      }
    }
    fclose(src); fclose(dst);
    return true;
#endif
  }

  X_FILESYSTEM_API bool x_fs_file_rename(const char* file, const char* newFile)
  {
#ifdef _WIN32
    return MoveFile(file, newFile) != 0;
#else
    return rename(file, newFile) == 0;
#endif
  }

  X_FILESYSTEM_API bool x_fs_directory_create(const char* path)
  {
#ifdef _WIN32
    return CreateDirectory(path, NULL) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
#else
    return mkdir(path, 0755) == 0 || errno == EEXIST;
#endif
  }

  X_FILESYSTEM_API bool x_fs_directory_create_recursive(const char* path)
  {
    size_t length = strlen(path);
    if (length >= X_FS_PAHT_MAX_LENGTH)
    {
      return false;
    }

    char temp_path[X_FS_PAHT_MAX_LENGTH];
    strncpy(temp_path, path, length);
    temp_path[length] = 0;

    char* p = temp_path;
#ifdef _WIN32
    if (p[1] == ':')
    { p += 2; }
#endif

    for (; *p; p++)
    {
      if (*p == '/' || *p == '\\')
      {
        char old = *p;
        *p = 0;
        x_fs_directory_create(temp_path);
        *p = old;
      }
    }

    return x_fs_directory_create(temp_path);
  }

  X_FILESYSTEM_API bool x_fs_directory_delete(const char* directory)
  {
#ifdef _WIN32
    return RemoveDirectory(directory) != 0;
#else
    return rmdir(directory) == 0;
#endif
  }

  X_FILESYSTEM_API bool x_fs_path_is_file(const XFSPath* path)
  {
    return x_fs_path_is_file_cstr(x_fs_path_cstr(path));
  }

  X_FILESYSTEM_API bool x_fs_path_is_file_cstr(const char* path)
  {
#ifdef _WIN32
    DWORD attributes = GetFileAttributes(path);
    return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat s;
    return stat(path, &s) == 0 && S_ISREG(s.st_mode);
#endif
  }

  X_FILESYSTEM_API bool x_fs_path_is_directory_cstr(const char* path)
  {
#ifdef _WIN32
    DWORD attributes = GetFileAttributes(path);
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat s;
    return stat(path, &s) == 0 && S_ISDIR(s.st_mode);
#endif
  }

  X_FILESYSTEM_API bool x_fs_path_is_directory(const XFSPath* path)
  {
    return x_fs_path_is_directory_cstr(x_fs_path_cstr(path));
  }

  X_FILESYSTEM_API void x_fs_path_clone(XFSPath* out, const XFSPath* path)
  {
    memcpy(out, path, sizeof(XFSPath));
  }

  X_FILESYSTEM_API XFSDireHandle* x_fs_find_first_file(const char* path, XFSDireEntry* entry)
  {
    XFSDireHandle* dir_handle = X_FILESYSTEM_ALLOC(sizeof(XFSDireHandle));
    if (!dir_handle) return NULL;

#ifdef _WIN32
    char searchPath[X_FS_MAX_PATH];
    snprintf(searchPath, sizeof(searchPath), "%s\\*", path);

    dir_handle->handle = FindFirstFile(searchPath, &dir_handle->findData);
    if (dir_handle->handle == INVALID_HANDLE_VALUE)
    {
      X_FILESYSTEM_FREE(dir_handle);
      return NULL;
    }

    // Fill DirectoryEntry with the first entry
    strncpy(entry->name, dir_handle->findData.cFileName, X_FS_MAX_PATH);
    entry->size = ((size_t)dir_handle->findData.nFileSizeHigh << 32) | dir_handle->findData.nFileSizeLow;
    entry->last_modified = s_x_fs_filetime_to_time_t_(&dir_handle->findData.ftLastWriteTime);
    entry->is_directory = (dir_handle->findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

#else
    dir_handle->dir = opendir(path);
    if (!dir_handle->dir)
    {
      X_FILESYSTEM_FREE(dir_handle);
      return NULL;
    }

    snprintf(dir_handle->base_path, sizeof(dir_handle->base_path), "%s", path);

    // Read the first entry and fill DirectoryEntry
    dir_handle->entry = readdir(dir_handle->dir);
    if (dir_handle->entry)
    {
      snprintf(entry->name, X_FS_MAX_PATH, "%s", dir_handle->entry->d_name);

      // Get file stats to fill size and last modified time
      char fullPath[X_FS_MAX_PATH];
      snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->name);
      if (stat(fullPath, &dir_handle->fileStat) == 0)
      {
        entry->size = (size_t)dir_handle->fileStat.st_size;
        entry->last_modified = dir_handle->fileStat.st_mtime;
      }
      else
      {
        entry->size = 0;
        entry->last_modified = 0;
      }
      entry->is_directory = S_ISDIR(dir_handle->fileStat.st_mode) ? 1 : 0;
    }
#endif

    return dir_handle;
  }

  X_FILESYSTEM_API bool x_fs_find_next_file(XFSDireHandle* dir_handle, XFSDireEntry* entry)
  {
#ifdef _WIN32
    if (FindNextFile(dir_handle->handle, &dir_handle->findData))
    {
      strncpy(entry->name, dir_handle->findData.cFileName, X_FS_MAX_PATH);
      entry->size = ((size_t)dir_handle->findData.nFileSizeHigh << 32) | dir_handle->findData.nFileSizeLow;
      entry->last_modified = s_x_fs_filetime_to_time_t_(&dir_handle->findData.ftLastWriteTime);
      entry->is_directory = (dir_handle->findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
      return true;
    }
#else
    dir_handle->entry = readdir(dir_handle->dir);
    if (dir_handle->entry)
    {
      snprintf(entry->name, X_FS_MAX_PATH, "%s", dir_handle->entry->d_name);

      // Get file stats to fill size and last modified time
      char fullPath[X_FS_MAX_PATH];
      snprintf(fullPath, sizeof(fullPath), "%s/%s", dir_handle->base_path, entry->name);
      if (stat(fullPath, &dir_handle->fileStat) == 0)
      {
        entry->size = (size_t)dir_handle->fileStat.st_size;
        entry->last_modified = dir_handle->fileStat.st_mtime;
      }
      else
      {
        entry->size = 0;
        entry->last_modified = 0;
      }
      entry->is_directory = S_ISDIR(dir_handle->fileStat.st_mode) ? 1 : 0;
      return true;
    }
#endif
    return false;
  }

  X_FILESYSTEM_API void x_fs_find_close(XFSDireHandle* dir_handle)
  {
#ifdef _WIN32
    FindClose(dir_handle->handle);
#else
    closedir(dir_handle->dir);
#endif
    X_FILESYSTEM_FREE(dir_handle);
  }

  X_FILESYSTEM_API XFSWatch* x_fs_watch_open(const char* path)
  {
    if (!path) return NULL;

    XFSWatch* fw = X_FILESYSTEM_CALLOC(1, sizeof(XFSWatch));
    if (!fw) return NULL;

#ifdef _WIN32
    wchar_t wpath[X_FS_MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, X_FS_MAX_PATH);

    fw->dir = CreateFileW(wpath, FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);

    if (fw->dir == INVALID_HANDLE_VALUE) {
      X_FILESYSTEM_FREE(fw);
      return NULL;
    }

    fw->overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    ReadDirectoryChangesW(fw->dir, fw->buffer, sizeof(fw->buffer), TRUE,
        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
        NULL, &fw->overlapped, NULL);

#elif defined(__linux__)
    fw->fd = inotify_init1(IN_NONBLOCK);
    if (fw->fd < 0) {
      X_FILESYSTEM_FREE(fw);
      return NULL;
    }

    fw->wd = inotify_add_watch(fw->fd, path,
        IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO);

    if (fw->wd < 0) {
      close(fw->fd);
      X_FILESYSTEM_FREE(fw);
      return NULL;
    }

#else
#error "x_fs_watch_open: Unsupported platform"
#endif

    return fw;
  }

  X_FILESYSTEM_API void x_fs_watch_close(XFSWatch* fw)
  {
    if (!fw) return;

#ifdef _WIN32
    CancelIo(fw->dir);
    CloseHandle(fw->dir);
    CloseHandle(fw->overlapped.hEvent);
#elif defined(__linux__)
    inotify_rm_watch(fw->fd, fw->wd);
    close(fw->fd);
#endif

    X_FILESYSTEM_FREE(fw);
  }

  X_FILESYSTEM_API int32_t x_fs_watch_poll(XFSWatch* fw, XFSWatchEvent* out_events,int32_t max_events)
  {
    if (!fw || !out_events || max_events <= 0) return 0;

    int32_t count = 0;

#ifdef _WIN32
    DWORD bytes;

    if (!fw->ready) {
      if (!GetOverlappedResult(fw->dir, &fw->overlapped, &bytes, FALSE)) return 0;
      fw->last_bytes = bytes;
      fw->ready = 1;
    }

    BYTE* ptr = (BYTE*)fw->buffer;
    FILE_NOTIFY_INFORMATION* fni = NULL;
    char filename[X_FS_MAX_PATH];

    while (fw->last_bytes && count < max_events) {
      fni = (FILE_NOTIFY_INFORMATION*)ptr;

      int32_t len = WideCharToMultiByte(CP_UTF8, 0, fni->FileName,
          fni->FileNameLength / 2, filename, sizeof(filename) - 1, NULL, NULL);
      filename[len] = 0;

      XFSWatchEvent ev = { x_fs_watch_UNKNOWN, filename };

      switch (fni->Action) {
        case FILE_ACTION_ADDED: ev.action = x_fs_watch_CREATED; break;
        case FILE_ACTION_REMOVED: ev.action = x_fs_watch_DELETED; break;
        case FILE_ACTION_MODIFIED: ev.action = x_fs_watch_MODIFIED; break;
        case FILE_ACTION_RENAMED_OLD_NAME: ev.action = x_fs_watch_RENAMED_FROM; break;
        case FILE_ACTION_RENAMED_NEW_NAME: ev.action = x_fs_watch_RENAMED_TO; break;
      }

      out_events[count++] = ev;

      if (!fni->NextEntryOffset) break;
      ptr += fni->NextEntryOffset;
    }

    fw->ready = 0;
    ReadDirectoryChangesW(fw->dir, fw->buffer, sizeof(fw->buffer), TRUE,
        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
        NULL, &fw->overlapped, NULL);

#elif defined(__linux__)
    if (fw->offset >= fw->len) {
      fw->len = read(fw->fd, fw->buffer, sizeof(fw->buffer));
      fw->offset = 0;
      if (fw->len <= 0) return 0;
    }

    while (fw->offset < fw->len && count < max_events) {
      struct inotify_event* e = (struct inotify_event*)&fw->buffer[fw->offset];

      if (e->len > 0) {
        XFSWatchEvent ev = { x_fs_watch_UNKNOWN, e->name };
        if (e->mask & IN_CREATE)     ev.action = x_fs_watch_CREATED;
        if (e->mask & IN_DELETE)     ev.action = x_fs_watch_DELETED;
        if (e->mask & IN_MODIFY)     ev.action = x_fs_watch_MODIFIED;
        if (e->mask & IN_MOVED_FROM) ev.action = x_fs_watch_RENAMED_FROM;
        if (e->mask & IN_MOVED_TO)   ev.action = x_fs_watch_RENAMED_TO;

        out_events[count++] = ev;
      }

      fw->offset += sizeof(struct inotify_event) + e->len;
    }

#else
#error "x_fs_watch_poll: Unsupported platform"
#endif

    return count;
  }

  X_FILESYSTEM_API size_t x_fs_get_temp_folder(XFSPath* out)
  {
#ifdef _WIN32
    // On Windows, use GetTempPath to retrieve the temporary folder path
    DWORD path_len = GetTempPathA((DWORD) X_FS_PAHT_MAX_LENGTH, out->buf);
    if (path_len == 0 || path_len > X_FS_PAHT_MAX_LENGTH)
    {
      return 0;
    }
    out->length = path_len;
#else
    // On Unix-based systems, check environment variables for temporary folder path
    const char *tmp_dir = getenv("TMPDIR");
    if (!tmp_dir) tmp_dir = getenv("TEMP");
    if (!tmp_dir) tmp_dir = getenv("TMP");
    if (!tmp_dir) tmp_dir = "/tmp"; // Default to /tmp if no environment variable is set

    // Copy the temporary folder path to buffer
    if (strlen(tmp_dir) >= X_FS_PAHT_MAX_LENGTH)
    {
      return 0;
    }
    x_fs_path(out, tmp_dir);
#endif

    return out->length;
  }

  X_FILESYSTEM_API static inline int32_t is_path_separator(char c)
  {
    return (c == X_FS_ALT_PATH_SEPARATOR || c == X_FS_PATH_SEPARATOR);
  }

  X_FILESYSTEM_API static inline int32_t pathchar_eq(char a, char b)
  {
    if (a == X_FS_ALT_PATH_SEPARATOR) a = X_FS_PATH_SEPARATOR;
    if (b == X_FS_ALT_PATH_SEPARATOR) b = X_FS_PATH_SEPARATOR;
    return a == b;
  }

  X_FILESYSTEM_API static const char* find_last_path_separator(const char* path)
  {
    const char* last_slash = strrchr(path, '/');
    const char* last_backslash = strrchr(path, '\\');
    return (last_slash > last_backslash) ? last_slash : last_backslash;
  }

  X_FILESYSTEM_API static inline char normalized_path_char(char c)
  {
    return (c == X_FS_ALT_PATH_SEPARATOR) ? X_FS_PATH_SEPARATOR : c;
  }

  // Helper: trim trailing separators (does not modify original)
  X_FILESYSTEM_API static size_t trim_trailing_separators(const char* buf, size_t len)
  {
    while (len > 0 && (buf[len - 1] == '/' || buf[len - 1] == '\\'))
    {
      --len;
    }
    return len;
  }

  X_FILESYSTEM_API void x_fs_path_init(XFSPath* p)
  {
    x_smallstr_clear(p);
  }

  X_FILESYSTEM_API static bool x_fs_utf8_validate(const char* str, size_t len)
  {
    size_t i = 0;
    while (i < len) {
      unsigned char c = (unsigned char)str[i];
      size_t remaining = len - i;

      if (c <= 0x7F) { // 1-byte ASCII
        i += 1;
      } else if ((c & 0xE0) == 0xC0) { // 2-byte
        if (remaining < 2 || (str[i + 1] & 0xC0) != 0x80 ||
            (c & 0xFE) == 0xC0) return false; // overlong enc
        i += 2;
      } else if ((c & 0xF0) == 0xE0) { // 3-byte
        if (remaining < 3 || 
            (str[i + 1] & 0xC0) != 0x80 || 
            (str[i + 2] & 0xC0) != 0x80) return false;
        unsigned char c1 = str[i + 1];
        if (c == 0xE0 && (c1 & 0xE0) == 0x80) return false; // overlong
        if (c == 0xED && c1 >= 0xA0) return false; // UTF-16 surrogate
        i += 3;
      } else if ((c & 0xF8) == 0xF0) { // 4-byte
        if (remaining < 4 || 
            (str[i + 1] & 0xC0) != 0x80 || 
            (str[i + 2] & 0xC0) != 0x80 || 
            (str[i + 3] & 0xC0) != 0x80) return false;
        unsigned char c1 = str[i + 1];
        if (c == 0xF0 && (c1 & 0xF0) == 0x80) return false; // overlong
        if (c == 0xF4 && c1 > 0x8F) return false; // > U+10FFFF
        if (c > 0xF4) return false;
        i += 4;
      } else {
        return false;
      }
    }
    return true;
  }

  X_FILESYSTEM_API size_t x_fs_path_set(XFSPath* p, const char* cstr)
  {
    size_t len = strlen(cstr);
    if (!x_fs_utf8_validate(cstr, len)) return 0;
    return x_smallstr_from_cstr(p, cstr);
  }

  X_FILESYSTEM_API size_t x_fs_path_set_slice(XFSPath* p, XSlice slice)
  {
    if (!x_fs_utf8_validate(slice.ptr, slice.length)) return 0;
    return x_smallstr_from_slice(slice, p);
  }

  X_FILESYSTEM_API size_t x_fs_path_append(XFSPath* p, const char* comp)
  {
    if (p->length > 0 && p->buf[p->length - 1] != X_FS_PATH_SEPARATOR)
    {
      if (x_smallstr_append_char(p, X_FS_PATH_SEPARATOR) != 0) return 0;
    }
    return x_smallstr_append_cstr(p, comp);
  }

  X_FILESYSTEM_API const char* x_fs_path_cstr(const XFSPath* p)
  {
    return x_smallstr_cstr(p);
  }

  X_FILESYSTEM_API size_t x_fs_path_join_(XFSPath* path, ...)
  {
    va_list args;
    va_start(args, path);

    if (path == NULL)
      return 0;

    bool join_success = true;

    const char* segment = NULL;
    while (join_success && (segment = va_arg(args, const char*)) != NULL)
    {
      join_success &= (x_fs_path_join_one(path, segment) > 0);
    }
    va_end(args);

    if (!join_success)
      return 0;
    return path->length;
  }

  X_FILESYSTEM_API size_t x_fs_path_join_slice_(XFSPath* path, ...)
  {
    va_list args;
    va_start(args, path);

    if (path == NULL)
      return 0;

    bool join_success = true;

    const XSlice* segment = NULL;
    while (join_success && (segment = va_arg(args, const XSlice*)) != NULL)
    {
      join_success &= (x_fs_path_join_one_slice(path, *segment) > 0);
    }
    va_end(args);

    if (!join_success)
      return 0;
    return path->length;
  }

  X_FILESYSTEM_API XSlice x_fs_path_as_slice(const XFSPath* path)
  {
    XSlice s;
    s.ptr = path ? path->buf : 0;
    s.length = path ? path->length : 0;
    return s;
  }

  X_FILESYSTEM_API XFSPath* x_fs_path_normalize(XFSPath* input)
  {
    if (!input)
      return NULL;

    XSmallstr temp;
    x_smallstr_clear(&temp);

    // Normalize separators in the input XSmallstr buffer (in place)
    for (size_t i = 0; i < input->length; ++i)
    {
      if (input->buf[i] == X_FS_ALT_PATH_SEPARATOR)
        input->buf[i] = X_FS_PATH_SEPARATOR;
    }

    size_t i = 0;
    size_t in_len = input->length;

    // Handle Windows drive prefix (e.g., C:\)
    if (in_len >= 2 && input->buf[1] == ':' && 
        (input->buf[2] == X_FS_PATH_SEPARATOR || input->buf[2] == '\0'))
    {
      x_smallstr_append_char(&temp, input->buf[0]);
      x_smallstr_append_char(&temp, ':');
      i = 2;
    } else if (in_len > 0 && input->buf[0] == X_FS_PATH_SEPARATOR)
    {
      x_smallstr_append_char(&temp, X_FS_PATH_SEPARATOR);
      i = 1;
    }

    size_t component_starts[128];
    size_t depth = 0;

    while (i <= in_len)
    {
      size_t start = i;
      while (i < in_len && input->buf[i] != X_FS_PATH_SEPARATOR) ++i;

      size_t len = i - start;

      if (len == 0 || (len == 1 && input->buf[start] == '.'))
      {
        // skip empty or "."
      } else if (len == 2 && input->buf[start] == '.' && input->buf[start + 1] == '.')
      {
        // ".." — pop last component if any
        if (depth > 0)
        {
          temp.length = component_starts[--depth];
          temp.buf[temp.length] = '\0';
        }
      } else
      {
        if (temp.length > 0 && temp.buf[temp.length - 1] != X_FS_PATH_SEPARATOR)
        {
          x_smallstr_append_char(&temp, X_FS_PATH_SEPARATOR);
        }
        component_starts[depth++] = temp.length;
        for (size_t j = 0; j < len; ++j)
        {
          x_smallstr_append_char(&temp, input->buf[start + j]);
        }
      }

      if (i < in_len && input->buf[i] == X_FS_PATH_SEPARATOR) ++i;
      else break;
    }

    // I assume normalized paths are always smaller than non-normalized paths
    // if (temp.length > X_x_smallstr_MAX_LENGTH)
    //  return -1;

    input->length = temp.length;
    memcpy(input->buf, temp.buf, temp.length);
    input->buf[temp.length] = '\0';

    return input;
  }

  X_FILESYSTEM_API XSlice x_fs_path_stem_cstr(const char* input)
  {
    XSlice empty = {0};
    if (!input)
      return empty;

    size_t len = strlen(input);

    const char* p = input + len - 1;
    const char* start = input;

    while (p > start && *p != '.') { p--; }
    return x_slice_init(input, p - input);
  }

  X_FILESYSTEM_API XSlice x_fs_path_basename_cstr(const char* input)
  {
    XSlice empty = {0};
    if (!input)
      return empty;

    const char* last_sep = find_last_path_separator(input);
    return x_slice(last_sep ? last_sep + 1 : input);
  }

  X_FILESYSTEM_API XSlice x_fs_path_dirname_cstr(const char* input)
  {
    XSlice empty =
    {0};
    if (!input) return empty;

    size_t input_len = strlen(input);
    if (input_len == 0)
      return empty;

    // Find the last slash or backslash
    const char* last_sep = find_last_path_separator(input);
    if (!last_sep)
      return empty;

    size_t len = (size_t)(last_sep - input);

    if (len == 0)
    {
      // Root case: "/file" → "/"
      char root[2] =
      { *last_sep, '\0' };
      return (XSlice){.ptr = root, .length = 1 };
    }

    return (XSlice){.ptr = input, .length = len };
  }

  X_FILESYSTEM_API XSlice x_fs_path_extension_cstr(const char* input)
  {
    const char* dot = strrchr(input, '.');
    if (!dot || strrchr(input, X_FS_PATH_SEPARATOR) > dot)
      return x_slice("");

    return x_slice(dot + 1);
  }

  X_FILESYSTEM_API size_t x_fs_path_change_extension(XFSPath* path, const char* new_ext)
  {
    if (!path || !new_ext) return 0;

    const char* path_str = x_smallstr_cstr(path);
    const char* last_dot = NULL;
    const char* last_sep = NULL;
    const size_t new_ext_len = strlen(new_ext);

    // Scan from the end to locate '.' and path separator
    for (ptrdiff_t i = (ptrdiff_t)(path->length - 1); i >= 0; --i)
    {
      char c = path_str[i];
      if (!last_dot && c == '.')
      {
        last_dot = path_str + i;
      }
      if (!last_sep && (c == '/' || c == '\\'))
      {
        last_sep = path_str + i;
        break;  // We can stop early once we find separator
      }
    }

    size_t base_len = (last_dot && (!last_sep || last_dot > last_sep))
      ? (size_t)(last_dot - path_str)
      : path->length;

    // Truncate existing path to remove current extension
    path->buf[base_len] = '\0';
    path->length = base_len;

    // Append the new extension
    if (new_ext[0] != '.' && new_ext_len > 0)
    {
      if (x_smallstr_append_char(path, '.') <= 0)
        return 0;
    }

    return x_smallstr_append_cstr(path, new_ext);
  }


  X_FILESYSTEM_API bool x_fs_path_is_absolute_native_cstr(const char* path)
  {
#ifdef _WIN32
    const char* p = path;
    size_t length = strlen(path);
    if (length >= 3)
      return (isalpha(p[0]) && p[1] == ':' && (p[2] == '\\' || p[2] == '/')) || (p[0] == '\\' && p[1] == '\\');
    else
      return (isalpha(p[0]) && p[1] == ':') || (p[0] == '\\' && p[1] == '\\');
#else
    return path[0] == '/';
#endif
  }

  X_FILESYSTEM_API bool x_fs_path_is_absolute_native(const XFSPath* path)
  {
    return x_fs_path_is_absolute_native_cstr(x_fs_path_cstr(path));
  }

  X_FILESYSTEM_API static inline bool x_fs_path_eq_cstr_cstr(const char* a, const char* b)
  {
    if (!a || !b)
      return false;

    while (*a && *b)
    {
      char ca = is_path_separator(*a) ? '/' : *a;
      char cb = is_path_separator(*b) ? '/' : *b;
      if (ca != cb)
        break;
      ++a;
      ++b;
    }

    // Skip trailing slashes or backslashes
    while (*a && is_path_separator(*a)) ++a;
    while (*b && is_path_separator(*b)) ++b;

    return (*a == '\0' && *b == '\0');
  }

  X_FILESYSTEM_API bool x_fs_path_eq(const XFSPath* path_a, const XFSPath* path_b)
  {
    if (path_a->buf == NULL && path_b->buf == NULL)
      return true;

    if (path_a->length == 0 && path_b->length == 0)
      return true;

    const char* a = &path_a->buf[0];
    const char* b = &path_b->buf[0];
    return x_fs_path_eq_cstr_cstr(a, b);
  }

  X_FILESYSTEM_API bool x_fs_path_eq_cstr(const XFSPath* path_a, const char* path_b)
  {
    if (path_a->buf == NULL && path_b == NULL)
      return false;

    const char* a = &(path_a->buf[0]);
    return x_fs_path_eq_cstr_cstr(a, path_b);
  }

  X_FILESYSTEM_API bool x_fs_path_is_relative(const XFSPath* path)
  {
    return !x_fs_path_is_absolute_cstr(path->buf);
  }

  X_FILESYSTEM_API bool x_fs_path_is_relative_cstr(const char* path)
  {
    return !x_fs_path_is_absolute_cstr(path);
  }

  X_FILESYSTEM_API bool x_fs_path_is_absolute_cstr(const char* path)
  {
    if (!path || !*path) return false;

    // POSIX absolute path
    if (path[0] == '/') return true;

    // Windows absolute path: e.g., "C:\..." or "C:/..."
    if (((path[0] >= 'A' && path[0] <= 'Z') ||
          (path[0] >= 'a' && path[0] <= 'z')) &&
        path[1] == ':' &&
        (path[2] == '/' || path[2] == '\\'))
      return true;

    return false;
  }

  X_FILESYSTEM_API bool x_fs_path_is_absolute(const XFSPath* path)
  {
    return x_fs_path_is_absolute_cstr(x_fs_path_cstr(path));
  }

  X_FILESYSTEM_API bool x_fs_path_common_prefix(const char* from_path, const char* to_path, XFSPath* out_path)
  {
    if (!from_path || !to_path || !out_path) return false;

    XFSPath from_copy;
    x_fs_path_init(&from_copy);

    size_t from_len = strlen(from_path);
    if (from_len > X_SMALLSTR_MAX_LENGTH)
      from_len = X_SMALLSTR_MAX_LENGTH;

    // We copy from_path to get rid of trailing path separators
    strncpy((char*)from_copy.buf, from_path, from_len);
    from_copy.buf[from_len] = '\0';
    from_copy.length = from_len;

    // Trim trailing separators
    while (from_copy.length > 0 && is_path_separator(from_copy.buf[from_copy.length - 1]))
    {
      from_copy.length--;
      from_copy.buf[from_copy.length] = '\0';
    }

    size_t prefix_len = from_copy.length;
    size_t to_len = strlen(to_path);

    if (to_len >= prefix_len)
    {
      if (strncmp((const char*)from_copy.buf, to_path, prefix_len) == 0)
      {
        if (to_len == prefix_len)
        {
          // Paths are identical
          x_fs_path_init(out_path);
          x_smallstr_from_cstr(out_path, ".");
          return true;
        }
        if (is_path_separator(to_path[prefix_len]))
        {
          // Return suffix after prefix + separator
          const char* suffix = to_path + prefix_len + 1;
          x_fs_path_init(out_path);
          x_smallstr_from_cstr(out_path, suffix);
          return true;
        }
      }
    }

    // No prefix match
    return false;
  }

  // Input must be normalized
  X_FILESYSTEM_API bool x_fs_path_split(const char* input, XFSPath* out_components, size_t max_components, size_t* out_count)
  {
    if (!input || !out_components || !out_count) return false;

    size_t len = strlen(input);
    size_t count = 0;
    size_t start = 0;

    for (size_t i = 0; i <= len; ++i)
    {
      if (i == len || is_path_separator(input[i]))
      {
        if (i > start)
        {
          if (count >= max_components) return false;

          size_t part_len = i - start;
          XSmallstr tmp;
          tmp.length = (part_len > X_SMALLSTR_MAX_LENGTH) ? X_SMALLSTR_MAX_LENGTH : part_len;
          strncpy((char*)tmp.buf, input + start, tmp.length);
          tmp.buf[tmp.length] = '\0';

          x_smallstr_from_cstr(&out_components[count], (const char*)tmp.buf);
          count++;
        }
        start = i + 1;
      }
    }

    *out_count = count;
    return true;
  }

  X_FILESYSTEM_API size_t x_fs_path_compare(const XFSPath* a, const XFSPath* b)
  {
    if (!a || !b) return 0;

    size_t alen = trim_trailing_separators((const char*) a->buf, a->length);
    size_t blen = trim_trailing_separators((const char*) b->buf, b->length);
    size_t min_len = (alen < blen) ? alen : blen;

    for (size_t i = 0; i < min_len; ++i)
    {
      char ca = normalized_path_char(a->buf[i]);
      char cb = normalized_path_char(b->buf[i]);
      if (ca != cb) return (unsigned char)ca - (unsigned char)cb;
    }

    return (int)(alen - blen);
  }

  X_FILESYSTEM_API int32_t x_fs_path_compare_cstr(const XFSPath* a, const char* cstr)
  {
    if (!a || !cstr) return -1;

    size_t clen = strlen(cstr);
    while (clen > 0 && (cstr[clen - 1] == '/' || cstr[clen - 1] == '\\'))
    {
      --clen;
    }

    size_t alen = trim_trailing_separators(a->buf, a->length);
    size_t min_len = (alen < clen) ? alen : clen;

    for (size_t i = 0; i < min_len; ++i)
    {
      char ca = normalized_path_char(a->buf[i]);
      char cb = normalized_path_char(cstr[i]);
      if (ca != cb) return (unsigned char)ca - (unsigned char)cb;
    }

    return (int)(alen - clen);
  }

  X_FILESYSTEM_API bool x_fs_path_exists_cstr(const char* path)
  {
#if defined(_WIN32) || defined(_WIN64)
#define x_fs_access _access
#define x_fs_f_ok 0
#else
#define x_fs_access access
#define x_fs_f_ok F_OK
#endif
    return x_fs_access(path, x_fs_f_ok) == 0;
  }

  X_FILESYSTEM_API bool x_fs_path_exists(const XFSPath* path)
  {
    return x_fs_path_exists_cstr(x_fs_path_cstr(path));
  }

  X_FILESYSTEM_API bool x_fs_path_exists_quick_cstr(const char* path)
  {
#ifdef _WIN32
    DWORD attributes = GetFileAttributes(path);
    return attributes != INVALID_FILE_ATTRIBUTES;
#else
    return access(path, F_OK) == 0;
#endif
  }

  X_FILESYSTEM_API bool x_fs_path_exists_quick(const XFSPath* path)
  {
    return x_fs_path_exists_quick_cstr(x_fs_path_cstr(path));
  }

  X_FILESYSTEM_API size_t x_fs_path_from_slice(XSlice sv, XFSPath* out)
  {
    return x_smallstr_from_slice(sv, out);
  }

  X_FILESYSTEM_API bool x_fs_file_stat(const char* path, FSFileStat* out_stat)
  {
    if (!path || !out_stat) return false;

#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &data))
    {
      return false;
    }
    out_stat->size = ((size_t)data.nFileSizeHigh << 32) | data.nFileSizeLow;

    FILETIME ft = data.ftCreationTime;
    ULARGE_INTEGER ui;
    ui.LowPart = ft.dwLowDateTime;
    ui.HighPart = ft.dwHighDateTime;
    out_stat->creation_time = (time_t)((ui.QuadPart - 116444736000000000ULL) / 10000000ULL);

    ft = data.ftLastWriteTime;
    ui.LowPart = ft.dwLowDateTime;
    ui.HighPart = ft.dwHighDateTime;
    out_stat->modification_time = (time_t)((ui.QuadPart - 116444736000000000ULL) / 10000000ULL);

    out_stat->permissions = data.dwFileAttributes;
#else
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    out_stat->size = (size_t)st.st_size;
    out_stat->creation_time = st.st_ctime;
    out_stat->modification_time = st.st_mtime;
    out_stat->permissions = st.st_mode;
#endif
    return true;
  }

  X_FILESYSTEM_API bool x_fs_file_modification_time(const char* path, time_t* out_time)
  {
    FSFileStat stat;
    if (x_fs_file_stat(path, &stat) == false) return false;
    *out_time = stat.modification_time;
    return true;
  }

  X_FILESYSTEM_API bool x_fs_file_creation_time(const char* path, time_t* out_time)
  {
    FSFileStat stat;
    if (x_fs_file_stat(path, &stat) == false) return false;
    *out_time = stat.creation_time;
    return true;
  }

  X_FILESYSTEM_API bool x_fs_file_permissions(const char* path, uint32_t* out_permissions)
  {
    FSFileStat stat;
    if (x_fs_file_stat(path, &stat) == false) return false;
    *out_permissions = stat.permissions;
    return true;
  }

  X_FILESYSTEM_API bool x_fs_file_set_permissions(const char* path, uint32_t permissions)
  {
#ifdef _WIN32
    return SetFileAttributesA(path, permissions);
#else
    return chmod(path, permissions) == 0;
#endif
  }

  X_FILESYSTEM_API bool x_fs_is_file(const char* path)
  {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    return (stat(path, &st) == 0) && S_ISREG(st.st_mode);
#endif
  }

  X_FILESYSTEM_API bool x_fs_is_directory(const char* path)
  {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    return (stat(path, &st) == 0) && S_ISDIR(st.st_mode);
#endif
  }

  X_FILESYSTEM_API bool x_fs_is_symlink(const char* path)
  {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_REPARSE_POINT);
#else
    struct stat st;
    return (lstat(path, &st) == 0) && S_ISLNK(st.st_mode);
#endif
  }

  X_FILESYSTEM_API bool x_fs_read_symlink(const char* path, XFSPath* out_path)
  {
#ifdef _WIN32
    // No direct equivalent in Win32 API; require fallback with GetFinalPathNameByHandle or similar
    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    char buf[X_FS_MAX_PATH];
    DWORD len = GetFinalPathNameByHandleA(hFile, buf, X_FS_MAX_PATH, FILE_NAME_NORMALIZED);
    CloseHandle(hFile);
    if (len == 0 || len >= X_FS_MAX_PATH) return false;
    return x_smallstr_from_cstr(out_path, buf);
#else
    char buf[PATH_MAX];
    ssize_t len = readlink(path, buf, sizeof(buf) - 1);
    if (len == -1) return false;
    buf[len] = '\0';
    return x_smallstr_from_cstr(out_path, buf) > 0;
#endif
  }

  X_FILESYSTEM_API bool x_fs_make_temp_file(const char* prefix, XFSPath* out_path)
  {
#ifdef _WIN32
    char tmpPath[X_FS_MAX_PATH];
    if (!GetTempPathA(X_FS_MAX_PATH, tmpPath)) return false;

    char tmpFile[X_FS_MAX_PATH];
    if (!GetTempFileNameA(tmpPath, prefix, 0, tmpFile)) return false;
    return x_smallstr_from_cstr(out_path, tmpFile);
#else
    char tmpl[PATH_MAX];
    snprintf(tmpl, sizeof(tmpl), "/tmp/%sXXXXXX", prefix);
    int32_t fd = mkstemp(tmpl);
    if (fd == -1) return false;
    close(fd);
    return x_smallstr_from_cstr(out_path, tmpl) > 0;
#endif
  }

  X_FILESYSTEM_API bool x_fs_make_temp_directory(const char* prefix, XFSPath* out_path)
  {
#ifdef _WIN32
    char tmpPath[X_FS_MAX_PATH];
    if (!GetTempPathA(X_FS_MAX_PATH, tmpPath)) return false;

    char dirPath[X_FS_MAX_PATH];
    for (int i = 0; i < 100; ++i)
    {
      snprintf(dirPath, sizeof(dirPath), "%s%s%d", tmpPath, prefix, i);
      if (CreateDirectoryA(dirPath, NULL))
      {
        return x_smallstr_from_cstr(out_path, dirPath);
      }
    }
    return false;
#else
    char tmpl[PATH_MAX];
    snprintf(tmpl, sizeof(tmpl), "/tmp/%sXXXXXX", prefix);
    if (!mkdtemp(tmpl)) return -1;
    return x_smallstr_from_cstr(out_path, tmpl) > 0;
#endif
  }

  X_FILESYSTEM_API XSlice x_fs_path_stem_as_slice(const XFSPath* input)
  {
    return x_fs_path_stem_cstr(x_fs_path_cstr(input));
  }

  X_FILESYSTEM_API size_t x_fs_path_stem(const XFSPath* input, XFSPath* out)
  {
    if (out == input)
    {
      XFSPath tmp;
      x_fs_path_init(&tmp);

      size_t n = x_fs_path_from_slice(x_fs_path_stem_as_slice(input), &tmp);
      if (n == 0)
      {
        return 0;
      }

      *out = tmp;
      return n;
    }

    return x_fs_path_from_slice(x_fs_path_stem_as_slice(input), out);
  }

  X_FILESYSTEM_API XSlice x_fs_path_basename_as_slice(const XFSPath* input)
  {
    return x_fs_path_basename_cstr(x_fs_path_cstr(input));
  }

  X_FILESYSTEM_API size_t x_fs_path_basename(const XFSPath* input, XFSPath* out)
  {
    if (out == input)
    {
      XFSPath tmp;
      x_fs_path_init(&tmp);

      size_t n = x_fs_path_from_slice(x_fs_path_basename_as_slice(input), &tmp);
      if (n == 0)
      {
        return 0;
      }

      *out = tmp;
      return n;
    }

    return x_fs_path_from_slice(x_fs_path_basename_as_slice(input), out);
  }

  X_FILESYSTEM_API XSlice x_fs_path_dirname_as_slice(const XFSPath* input)
  {
    return x_fs_path_dirname_cstr(x_fs_path_cstr(input));
  }

  X_FILESYSTEM_API size_t x_fs_path_dirname(const XFSPath* input, XFSPath* out)
  {
    if (out == input)
    {
      XFSPath tmp;
      x_fs_path_init(&tmp);

      size_t n = x_fs_path_from_slice(x_fs_path_dirname_as_slice(input), &tmp);
      if (n == 0)
      {
        return 0;
      }

      *out = tmp;
      return n;
    }

    return x_fs_path_from_slice(x_fs_path_dirname_as_slice(input), out);
  }

  X_FILESYSTEM_API XSlice x_fs_path_extension_as_slice(const XFSPath* input)
  {
    return x_fs_path_extension_cstr(x_fs_path_cstr(input));
  }

  X_FILESYSTEM_API size_t x_fs_path_extension(const XFSPath* input, XFSPath* out)
  {
    if (out == input)
    {
      XFSPath tmp;
      x_fs_path_init(&tmp);

      size_t n = x_fs_path_from_slice(x_fs_path_extension_as_slice(input), &tmp);
      if (n == 0)
      {
        return 0;
      }

      *out = tmp;
      return n;
    }

    return x_fs_path_from_slice(x_fs_path_extension_as_slice(input), out);
  }

  X_FILESYSTEM_API XFSTime x_fs_time_from_epoch(time_t t)
  {
    XFSTime out;
    struct tm tmv;

#if defined(_WIN32)
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif

    out.year   = tmv.tm_year + 1900;
    out.month  = tmv.tm_mon + 1;
    out.day    = tmv.tm_mday;
    out.hour   = tmv.tm_hour;
    out.minute = tmv.tm_min;
    out.second = tmv.tm_sec;

    return out;
  }

  //
  // Relative directory
  //

  static bool s_x_fs_is_separator(char c)
  {
    return c == '/' || c == '\\';
  }

  static char s_x_fs_fold_path_char(char c)
  {
#if defined(_WIN32)
    if (c >= 'A' && c <= 'Z')
    {
      return (char)(c - 'A' + 'a');
    }
#endif
    return c;
  }

  static bool s_x_fs_path_char_equal(char a, char b)
  {
    if (s_x_fs_is_separator(a) && s_x_fs_is_separator(b))
    {
      return true;
    }

    return s_x_fs_fold_path_char(a) == s_x_fs_fold_path_char(b);
  }

  static size_t s_x_fs_count_remaining_segments(const char* s)
  {
    size_t count = 0;
    bool in_segment = false;

    while (*s)
    {
      if (s_x_fs_is_separator(*s))
      {
        if (in_segment)
        {
          count++;
          in_segment = false;
        }
      }
      else
      {
        in_segment = true;
      }

      s++;
    }

    if (in_segment)
    {
      count++;
    }

    return count;
  }

  static size_t s_x_fs_find_common_prefix_boundary(const char* a, const char* b)
  {
    size_t i = 0;
    size_t last_boundary = 0;

    while (a[i] && b[i] && s_x_fs_path_char_equal(a[i], b[i]))
    {
      i++;

      if (!a[i] || !b[i])
      {
        break;
      }

      if (s_x_fs_is_separator(a[i - 1]) && s_x_fs_is_separator(b[i - 1]))
      {
        last_boundary = i;
      }
    }

    if (i > 0)
    {
      if (!a[i] && !b[i])
      {
        return i;
      }

      if (!a[i] && s_x_fs_is_separator(b[i]))
      {
        return i + 1;
      }

      if (!b[i] && s_x_fs_is_separator(a[i]))
      {
        return i + 1;
      }

      if (s_x_fs_is_separator(a[i]) && s_x_fs_is_separator(b[i]))
      {
        return i + 1;
      }
    }

    return last_boundary;
  }

  X_FILESYSTEM_API size_t x_fs_path_relative_to(const XFSPath* from_path, const XFSPath* to_path, XFSPath* out_path)
  {
    XFSPath from_norm;
    XFSPath to_norm;
    const char* from;
    const char* to;
    size_t common;
    size_t up_count;
    char tmp[sizeof(out_path->buf)];
    size_t pos = 0;
    size_t i;

    if (!from_path || !to_path || !out_path)
    {
      return 0;
    }

    x_fs_path_clone(&from_norm, from_path);
    x_fs_path_clone(&to_norm, to_path);

    x_fs_path_normalize(&from_norm);
    x_fs_path_normalize(&to_norm);

    from = from_norm.buf;
    to = to_norm.buf;

    common = s_x_fs_find_common_prefix_boundary(from, to);

    while (from[common] && s_x_fs_is_separator(from[common]))
    {
      common++;
    }

    while (to[common] && s_x_fs_is_separator(to[common]))
    {
      common++;
    }

    up_count = s_x_fs_count_remaining_segments(from + common);

    for (i = 0; i < up_count; i++)
    {
      if (pos + 3 >= sizeof(tmp))
      {
        return 0;
      }

      tmp[pos++] = '.';
      tmp[pos++] = '.';
      tmp[pos++] = '/';
    }

    while (to[common] && s_x_fs_is_separator(to[common]))
    {
      common++;
    }

    while (to[common])
    {
      char c = to[common++];

      if (pos + 1 >= sizeof(tmp))
      {
        return 0;
      }

      if (s_x_fs_is_separator(c))
      {
        c = '/';
      }

      tmp[pos++] = c;
    }

    if (pos == 0)
    {
      if (sizeof(tmp) < 2)
      {
        return 0;
      }

      tmp[pos++] = '.';
    }

    tmp[pos] = '\0';
    x_fs_path(out_path, tmp);
    return out_path->length;
  }

  X_FILESYSTEM_API size_t x_fs_path_relative_to_cstr(const char* from_path, const char* to_path, XFSPath* out_path)
  {
    XFSPath from;
    XFSPath to;

    if (!from_path || !to_path || !out_path)
    {
      return 0;
    }

    x_fs_path(&from, from_path);
    x_fs_path(&to, to_path);

    return x_fs_path_relative_to(&from, &to, out_path);
  }

#ifdef __cplusplus
}

#endif

#endif  // X_IMPL_FILESYSTEM

#ifdef X_INTERNAL_STRING_IMPL
#undef X_IMPL_STRING
#undef X_INTERNAL_STRING_IMPL
#endif
#endif  // X_FILESYSTEM_H
