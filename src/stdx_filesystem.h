/*
 * STDX - Filesystem Utilities
 * Part of the STDX General Purpose C Library by marciovmf
 * https://github.com/marciovmf/stdx
 *
 * Provides a cross-platform filesystem abstraction including:
 *   - Directory and file operations (create, delete, copy, rename, enumerate)
 *   - Filesystem monitoring via watch APIs
 *   - Rich path manipulation utilities (normalize, join, basename, dirname, extension, relative paths)
 *   - File metadata retrieval and modification (timestamps, permissions)
 *   - Utilities for symbolic links and file type queries
 *   - Temporary file and directory creation
 *   - Functions accepts and preserves valid UTF-8 (raw, not decodedint32_to codepoints) paths.
 *
 * Designed to unify and simplify filesystem operations across platforms.
 *
 * To compile the implementation, define:
 *     #define STDX_IMPLEMENTATION_FILESYSTEM
 * in **one** source file before including this header.
 *
 * Author: marciovmf
 * License: MIT
 * Dependencies: stdx_string.h
 * Usage: #include "stdx_filesystem.h"
 */

#ifndef STDX_FILESYSTEM_H
#define STDX_FILESYSTEM_H

#ifdef __cplusplus
extern "C"
{
#endif

#define STDX_FILESYSTEM_VERSION_MAJOR 1
#define STDX_FILESYSTEM_VERSION_MINOR 0
#define STDX_FILESYSTEM_VERSION_PATCH 0

#define STDX_FILESYSTEM_VERSION (STDX_FILESYSTEM_VERSION_MAJOR * 10000 + STDX_FILESYSTEM_VERSION_MINOR * 100 + STDX_FILESYSTEM_VERSION_PATCH)

#ifdef STDX_IMPLEMENTATION_FILESYSTEM
#ifndef STDX_IMPLEMENTATION_STRING
#define STDX_INTERNAL_STRING_IMPLEMENTATION
#define STDX_IMPLEMENTATION_STRING
#endif
#endif
#include <stdx_string.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#ifndef X_FS_PAHT_MAX_LENGTH
# define X_FS_PAHT_MAX_LENGTH 512
#endif  // X_FS_PAHT_MAX_LENGTH

#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#define ALT_SEPARATOR '/'
#else
#define PATH_SEPARATOR '/'
#define ALT_SEPARATOR '\\'
#endif

  typedef XSmallstr XFSPath;
  typedef struct XFSDireEntry_t XFSDireEntry;
  typedef struct XFSDireHandle_t XFSDireHandle;
  typedef struct XFSWatch_t XFSWatch;

  struct XFSDireEntry_t
  {
    int8_t name[X_FS_PAHT_MAX_LENGTH]; 
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
    const int8_t* filename; // Valid until next poll
  } XFSWatchEvent;


  // ---------------------------------------------------------------------------
  // Filesystem operations
  // ---------------------------------------------------------------------------

  XFSDireHandle* x_fs_find_first_file(const int8_t* path, XFSDireEntry* entry);
  bool        x_fs_directory_create(const int8_t* path);
  bool        x_fs_directory_create_recursive(const int8_t* path);
  bool        x_fs_directory_delete(const int8_t* directory);
  bool        x_fs_file_copy(const int8_t* file, const int8_t* newFile);
  bool        x_fs_file_rename(const int8_t* file, const int8_t* newFile);
  bool        x_fs_find_next_file(XFSDireHandle* dirHandle, XFSDireEntry* entry);
  size_t      x_fs_get_temp_folder(XFSPath* out);
  size_t      x_fs_cwd_get(XFSPath* path);
  size_t      x_fs_cwd_set(const int8_t* path);
  size_t      x_fs_cwd_set_from_executable_path(void);
  void        x_fs_find_close(XFSDireHandle* dirHandle);


  // ---------------------------------------------------------------------------
  // Filesystem Monitoring
  // ---------------------------------------------------------------------------

  XFSWatch*   x_fs_watch_open(const int8_t* path);
  void        x_fs_watch_close(XFSWatch* fw);
  int32_t         x_fs_watch_poll(XFSWatch* fw, XFSWatchEvent* out_events,int32_t max_events);


  // ---------------------------------------------------------------------------
  // Path manipulation
  // ---------------------------------------------------------------------------

#define       x_fs_path(out, ...) x_fs_path_(out, __VA_ARGS__, 0)
#define       x_fs_path_join(path, ...) x_fs_path_join_(path, __VA_ARGS__, 0)

  XFSPath*    x_fs_path_normalize(XFSPath* input);
  XStrview    x_fs_path_basename(const int8_t* input);
  XStrview    x_fs_path_dirname(const int8_t* input);
  bool        x_fs_path_(XFSPath* out, ...);
  bool        x_fs_path_exists(const XFSPath* path);
  bool        x_fs_path_exists_cstr(const int8_t* path);
  bool        x_fs_path_exists_quick(const XFSPath* path); // Sligthly faster
  bool        x_fs_path_exists_quick_cstr(const int8_t* path); // Sligthly faster
  bool        x_fs_path_is_absolute(const XFSPath* path);
  bool        x_fs_path_is_absolute_cstr(const int8_t* path);
  bool        x_fs_path_is_absolute_native(const XFSPath* path);
  bool        x_fs_path_is_absolute_native_cstr(const int8_t* path);
  bool        x_fs_path_is_directory(const XFSPath* path);
  bool        x_fs_path_is_directory_cstr(const int8_t* path);
  bool        x_fs_path_is_file(const XFSPath* path);
  bool        x_fs_path_is_file_cstr(const int8_t* path);
  bool        x_fs_path_is_relative(const XFSPath* path);
  bool        x_fs_path_is_relative_cstr(const int8_t* path);
  const int8_t* x_fs_path_cstr(const XFSPath* p);
  size_t      x_fs_path_append(XFSPath* p, const int8_t* comp);
  size_t      x_fs_path_change_extension(XFSPath* path, const int8_t* new_ext);
  int32_t         x_fs_path_compare(const XFSPath* a, const XFSPath* b); // ignores separator type
  int32_t         x_fs_path_compare_cstr(const XFSPath* a, const int8_t* cstr); // ignores separator type
  bool        x_fs_path_eq(const XFSPath* a, const XSmallstr* b);
  bool        x_fs_path_eq_cstr(const XFSPath* a, const int8_t* b);
  XStrview    x_fs_path_extension(const int8_t* input);
  size_t      x_fs_path_from_strview(XStrview sv, XFSPath* out);
  size_t      x_fs_path_join_(XFSPath* path, ...);
  size_t      x_fs_path_relative_cstr(const int8_t* from_path, const int8_t* to_path, XFSPath* out_path);
  size_t      x_fs_path_set(XFSPath* p, const int8_t* cstr);
  bool        x_fs_path_split(const int8_t* input, XFSPath* out_components, size_t max_components, size_t* out_count);
  size_t      x_fs_path_from_executable(XFSPath* out);


  // ---------------------------------------------------------------------------
  // Path manipulation
  // ---------------------------------------------------------------------------

#include <stddef.h>
  typedef struct
  {
    size_t size;
    time_t creation_time;
    time_t modification_time;
    uint32_t permissions; // Platform dependent
  } FSFileStat;


  // Retrieve file or directory metadata. Returns 0 on success.
  bool x_fs_file_stat(const int8_t* path, FSFileStat* out_stat);

  // Get last modification time. Returns 0 on success.
  bool x_fs_file_modification_time(const int8_t* path, time_t* out_time);

  // Get creation time. Returns 0 on success.
  bool x_fs_file_creation_time(const int8_t* path, time_t* out_time);

  // Get file permissions.
  bool x_fs_file_permissions(const int8_t* path, uint32_t* out_permissions);

  // Set file permissions.
  bool x_fs_file_set_permissions(const int8_t* path, uint32_t permissions);

  // Utility
  bool x_fs_is_file(const int8_t* path);
  bool x_fs_is_directory(const int8_t* path);
  bool x_fs_is_symlink(const int8_t* path);
  bool x_fs_read_symlink(const int8_t* path, XFSPath* out_path);

  // Temp files and dirs
  bool x_fs_make_temp_file(const int8_t* prefix, XFSPath* out_path);
  bool x_fs_make_temp_directory(const int8_t* prefix, XFSPath* out_path); 


#ifdef STDX_IMPLEMENTATION_FILESYSTEM

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

#include <time.h>

  struct XFSDireHandle_t
  {
#ifdef _WIN32
    HANDLE handle;
    WIN32_FIND_DATA findData;
#else
    DIR* dir;
    struct dirent* entry;
    struct stat fileStat;
#endif
  }; 

  struct XFSWatch_t
  {
#ifdef _WIN32
    HANDLE dir;
    OVERLAPPED overlapped;
    int8_t buffer[4096];
    DWORD last_bytes;
    int32_t ready;
#elif defined(__linux__)
    int32_t fd;
    int32_t wd;
    int8_t buffer[4096];
    int32_t offset, len;
#endif
  };




  int32_t x_fs_path_join_one(XFSPath* out, const int8_t* segment)
  {
    if (!out || !segment) return false;

    size_t seg_len = strlen(segment);
    if (seg_len == 0) return true; // Nothing to append

    bool needs_sep = false;

    if (out->length > 0 && out->buf[out->length - 1] != PATH_SEPARATOR) {
      needs_sep = true;
    }

    size_t total_needed = out->length + (needs_sep ? 1 : 0) + seg_len;

    if (total_needed > STDX_SMALLSTR_MAX_LENGTH) return false; // Not enough space

    // Add separator if needed
    if (needs_sep) {
      out->buf[out->length++] = PATH_SEPARATOR;
    }

    // Append segment
    memcpy(&out->buf[out->length], segment, seg_len);
    out->length += seg_len;

    // Null-terminate
    out->buf[out->length] = '\0';
    return (int) out->length;
  }

  bool x_fs_path_(XFSPath* out, ...)
  {
    va_list args;
    va_start(args, out);

    out->buf[0] = '\0';
    out->length = 0;

    const int8_t* segment = NULL;
    while ((segment = va_arg(args, const int8_t*)) != NULL)
    {
      size_t length = x_fs_path_join_one(out, segment);
      if (length >= X_FS_PAHT_MAX_LENGTH)
        return false;
    }

    va_end(args);
    return true;
  }

  size_t x_fs_cwd_get(XFSPath* path)
  {
#ifdef _WIN32
    DWORD size = GetCurrentDirectory(X_FS_PAHT_MAX_LENGTH, path->buf);
    return (size_t)size;
#else
    int8_t* result = getcwd(path->buf, X_FS_PAHT_MAX_LENGTH);
    return result ? strlen(path->buf) : 0;
#endif
  }

  size_t x_fs_cwd_set(const int8_t* path)
  {
#ifdef _WIN32
    return SetCurrentDirectory(path) != 0;
#else
    return chdir(path) == 0;
#endif
  }

  size_t x_fs_path_from_executable(XFSPath* out)
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

  size_t x_fs_cwd_set_from_executable_path(void)
  {
    XFSPath program_path;
    size_t bytesCopied = x_fs_path_from_executable(&program_path);
    x_fs_cwd_set(program_path.buf);
    return bytesCopied;
  }

  bool x_fs_file_copy(const int8_t* file, const int8_t* newFile)
  {
#ifdef _WIN32
    return CopyFile(file, newFile, FALSE) != 0;
#else
    FILE *src = fopen(file, "rb");
    if (!src) return false;
    FILE *dst = fopen(newFile, "wb");
    if (!dst)
    { fclose(src); return false; }

    int8_t buf[4096];
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

  bool x_fs_file_rename(const int8_t* file, const int8_t* newFile)
  {
#ifdef _WIN32
    return MoveFile(file, newFile) != 0;
#else
    return rename(file, newFile) == 0;
#endif
  }

  bool x_fs_directory_create(const int8_t* path)
  {
#ifdef _WIN32
    return CreateDirectory(path, NULL) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
#else
    return mkdir(path, 0755) == 0 || errno == EEXIST;
#endif
  }

  bool x_fs_directory_create_recursive(const int8_t* path)
  {
    size_t length = strlen(path);
    if (length >= X_FS_PAHT_MAX_LENGTH)
    {
      return false;
    }

    int8_t temp_path[X_FS_PAHT_MAX_LENGTH];
    strncpy(temp_path, path, length);
    temp_path[length] = 0;

    int8_t* p = temp_path;
#ifdef _WIN32
    if (p[1] == ':')
    { p += 2; }
#endif

    for (; *p; p++)
    {
      if (*p == '/' || *p == '\\')
      {
        int8_t old = *p;
        *p = 0;
        x_fs_directory_create(temp_path);
        *p = old;
      }
    }

    return x_fs_directory_create(temp_path);
  }

  bool x_fs_directory_delete(const int8_t* directory)
  {
#ifdef _WIN32
    return RemoveDirectory(directory) != 0;
#else
    return rmdir(directory) == 0;
#endif
  }

  bool x_fs_path_is_file(const XFSPath* path)
  {
    return x_fs_path_is_file_cstr(x_fs_path_cstr(path));
  }


  bool x_fs_path_is_file_cstr(const int8_t* path)
  {
#ifdef _WIN32
    DWORD attributes = GetFileAttributes(path);
    return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat s;
    return stat(path, &s) == 0 && S_ISREG(s.st_mode);
#endif
  }

  bool x_fs_path_is_directory_cstr(const int8_t* path)
  {
#ifdef _WIN32
    DWORD attributes = GetFileAttributes(path);
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat s;
    return stat(path, &s) == 0 && S_ISDIR(s.st_mode);
#endif
  }

  bool x_fs_path_is_directory(const XFSPath* path)
  {
    return x_fs_path_is_directory_cstr(x_fs_path_cstr(path));
  }

  void x_fs_path_clone(XFSPath* out, const XFSPath* path)
  {
    memcpy(out, path, sizeof(XFSPath));
  }

  XFSDireHandle* x_fs_find_first_file(const int8_t* path, XFSDireEntry* entry)
  {
    XFSDireHandle* dir_handle = malloc(sizeof(XFSDireHandle));
    if (!dir_handle) return NULL;

#ifdef _WIN32
    int8_t searchPath[MAX_PATH];
    snprintf(searchPath, sizeof(searchPath), "%s\\*", path);

    dir_handle->handle = FindFirstFile(searchPath, &dir_handle->findData);
    if (dir_handle->handle == INVALID_HANDLE_VALUE)

    {
      free(dir_handle);
      return NULL;
    }

    // Fill DirectoryEntry with the first entry
    strncpy(entry->name, dir_handle->findData.cFileName, MAX_PATH);
    entry->size = ((size_t)dir_handle->findData.nFileSizeHigh << 32) | dir_handle->findData.nFileSizeLow;
    entry->last_modified = *(time_t*)&dir_handle->findData.ftLastWriteTime;
    entry->is_directory = (dir_handle->findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

#else
    dir_handle->dir = opendir(path);
    if (!dir_handle->dir)

    {
      free(dir_handle);
      return NULL;
    }

    // Read the first entry and fill DirectoryEntry
    dir_handle->entry = readdir(dir_handle->dir);
    if (dir_handle->entry)

    {
      snprintf(entry->name, MAX_PATH, "%s", dir_handle->entry->d_name);

      // Get file stats to fill size and last modified time
      int8_t fullPath[MAX_PATH];
      snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->name);
      stat(fullPath, &dir_handle->fileStat);
      entry->size = dir_handle->fileStat.st_size;
      entry->last_modified = dir_handle->fileStat.st_mtime;
      entry->is_directory = (dir_handle->entry->d_type == DT_DIR);
    }
#endif

    return dir_handle;
  }

  bool x_fs_find_next_file(XFSDireHandle* dir_handle, XFSDireEntry* entry)
  {
#ifdef _WIN32
    if (FindNextFile(dir_handle->handle, &dir_handle->findData))
    {
      strncpy(entry->name, dir_handle->findData.cFileName, MAX_PATH);
      entry->size = ((size_t)dir_handle->findData.nFileSizeHigh << 32) | dir_handle->findData.nFileSizeLow;
      entry->last_modified = *(time_t*)&dir_handle->findData.ftLastWriteTime; // Convert to time_t
      entry->is_directory = (dir_handle->findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
      return true;
    }
#else
    dir_handle->entry = readdir(dir_handle->dir);
    if (dir_handle->entry)
    {
      snprintf(entry->name, MAX_PATH, "%s", dir_handle->entry->d_name);

      // Get file stats to fill size and last modified time
      int8_t fullPath[MAX_PATH];
      snprintf(fullPath, sizeof(fullPath), "%s/%s", dir_handle->entry->d_name, entry->name);
      stat(fullPath, &dir_handle->fileStat);
      entry->size = dir_handle->fileStat.st_size;
      entry->last_modified = dir_handle->fileStat.st_mtime;
      entry->is_directory = (dir_handle->entry->d_type == DT_DIR);
      return true;
    }
#endif
    return false;
  }

  void x_fs_find_close(XFSDireHandle* dir_handle)
  {
#ifdef _WIN32
    FindClose(dir_handle->handle);
#else
    closedir(dir_handle->dir);
#endif
    free(dir_handle);
  }

  XFSWatch* x_fs_watch_open(const int8_t* path)
  {
    if (!path) return NULL;

    XFSWatch* fw = calloc(1, sizeof(XFSWatch));
    if (!fw) return NULL;

#ifdef _WIN32
    wchar_t wpath[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, MAX_PATH);

    fw->dir = CreateFileW(wpath, FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);

    if (fw->dir == INVALID_HANDLE_VALUE) {
      free(fw);
      return NULL;
    }

    fw->overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    ReadDirectoryChangesW(fw->dir, fw->buffer, sizeof(fw->buffer), TRUE,
        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
        NULL, &fw->overlapped, NULL);

#elif defined(__linux__)
    fw->fd = inotify_init1(IN_NONBLOCK);
    if (fw->fd < 0) {
      free(fw);
      return NULL;
    }

    fw->wd = inotify_add_watch(fw->fd, path,
        IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO);

    if (fw->wd < 0) {
      close(fw->fd);
      free(fw);
      return NULL;
    }

#else
#error "x_fs_watch_open: Unsupported platform"
#endif

    return fw;
  }

  void x_fs_watch_close(XFSWatch* fw)
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

    free(fw);
  }

  int32_t x_fs_watch_poll(XFSWatch* fw, XFSWatchEvent* out_events,int32_t max_events)
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
    int8_t filename[MAX_PATH];

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

  size_t x_fs_get_temp_folder(XFSPath* out)
  {
#ifdef _WIN32
    // On Windows, use GetTempPath to retrieve the temporary folder path
    DWORD path_len = GetTempPathA((DWORD) X_FS_PAHT_MAX_LENGTH, out->buf);
    if (path_len == 0 || path_len > X_FS_PAHT_MAX_LENGTH)
    {
      return -1;
    }
    out->length = path_len;
#else
    // On Unix-based systems, check environment variables for temporary folder path
    const int8_t *tmp_dir = getenv("TMPDIR");
    if (!tmp_dir) tmp_dir = getenv("TEMP");
    if (!tmp_dir) tmp_dir = getenv("TMP");
    if (!tmp_dir) tmp_dir = "/tmp"; // Default to /tmp if no environment variable is set

    // Copy the temporary folder path to buffer
    if (strlen(tmp_dir) >= X_FS_PAHT_MAX_LENGTH)

    {
      return -1;
    }
    x_fs_path(out, tmp_dir);
#endif

    return out->length;
  }

  static inline int32_t is_path_separator(char c)
  {
    return (c == ALT_SEPARATOR || c == PATH_SEPARATOR);
  }

  static inline int32_t pathchar_eq(char a, int8_t b)
  {
    if (a == ALT_SEPARATOR) a = PATH_SEPARATOR;
    if (b == ALT_SEPARATOR) b = PATH_SEPARATOR;
    return a == b;
  }

  static const int8_t* find_last_path_separator(const int8_t* path)
  {
    const int8_t* last_slash = strrchr(path, '/');
    const int8_t* last_backslash = strrchr(path, '\\');
    return (last_slash > last_backslash) ? last_slash : last_backslash;
  }

  static inline int8_t normalized_path_char(char c)
  {
    return (c == ALT_SEPARATOR) ? PATH_SEPARATOR : c;
  }

  // Helper: trim trailing separators (does not modify original)
  static size_t trim_trailing_separators(const uint8_t* buf, size_t len)
  {
    while (len > 0 && (buf[len - 1] == '/' || buf[len - 1] == '\\'))
    {
      --len;
    }
    return len;
  }

  void x_fs_path_init(XFSPath* p)
  {
    x_smallstr_clear(p);
  }

  static bool x_fs_utf8_validate(const int8_t* str, size_t len)
  {
    size_t i = 0;
    while (i < len) {
      uint8_t c = (uint8_t)str[i];
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
        uint8_t c1 = str[i + 1];
        if (c == 0xE0 && (c1 & 0xE0) == 0x80) return false; // overlong
        if (c == 0xED && c1 >= 0xA0) return false; // UTF-16 surrogate
        i += 3;
      } else if ((c & 0xF8) == 0xF0) { // 4-byte
        if (remaining < 4 || 
            (str[i + 1] & 0xC0) != 0x80 || 
            (str[i + 2] & 0xC0) != 0x80 || 
            (str[i + 3] & 0xC0) != 0x80) return false;
        uint8_t c1 = str[i + 1];
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

  size_t x_fs_path_set(XFSPath* p, const int8_t* cstr)
  {
    size_t len = strlen(cstr);
    if (!x_fs_utf8_validate(cstr, len)) return -1;
    return x_smallstr_from_cstr(p, cstr);
  }

  size_t x_fs_path_append(XFSPath* p, const int8_t* comp)
  {
    if (p->length > 0 && p->buf[p->length - 1] != PATH_SEPARATOR)
    {
      if (x_smallstr_append_char(p, PATH_SEPARATOR) != 0) return -1;
    }
    return x_smallstr_append_cstr(p, comp);
  }

  const int8_t* x_fs_path_cstr(const XFSPath* p)
  {
    return x_smallstr_cstr(p);
  }

  size_t x_fs_path_join_(XFSPath* path, ...)
  {
    va_list args;
    va_start(args, path);

    if (path == NULL)
      return -1;

    bool join_success = true;

    const int8_t* segment = NULL;
    while (join_success && (segment = va_arg(args, const int8_t*)) != NULL)
    {
      join_success &= x_fs_path_join_one(path, segment);
    }
    va_end(args);

    if (!join_success)
      return -1;
    return (int) path->length;
  }

  XFSPath* x_fs_path_normalize(XFSPath* input)
  {
    if (!input)
      return NULL;

    XSmallstr temp;
    x_smallstr_clear(&temp);

    // Normalize separators in the input XSmallstr buffer (in place)
    for (size_t i = 0; i < input->length; ++i)

    {
      if (input->buf[i] == ALT_SEPARATOR)
        input->buf[i] = PATH_SEPARATOR;
    }

    size_t i = 0;
    size_t in_len = input->length;

    // Handle Windows drive prefix (e.g., C:\)
    if (in_len >= 2 && input->buf[1] == ':' && 
        (input->buf[2] == PATH_SEPARATOR || input->buf[2] == '\0'))

    {
      x_smallstr_append_char(&temp, input->buf[0]);
      x_smallstr_append_char(&temp, ':');
      i = 2;
    } else if (in_len > 0 && input->buf[0] == PATH_SEPARATOR)

    {
      x_smallstr_append_char(&temp, PATH_SEPARATOR);
      i = 1;
    }

    size_t component_starts[128];
    size_t depth = 0;

    while (i <= in_len)

    {
      size_t start = i;
      while (i < in_len && input->buf[i] != PATH_SEPARATOR) ++i;

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
        if (temp.length > 0 && temp.buf[temp.length - 1] != PATH_SEPARATOR)

        {
          x_smallstr_append_char(&temp, PATH_SEPARATOR);
        }
        component_starts[depth++] = temp.length;
        for (size_t j = 0; j < len; ++j)

        {
          x_smallstr_append_char(&temp, input->buf[start + j]);
        }
      }

      if (i < in_len && input->buf[i] == PATH_SEPARATOR) ++i;
      else break;
    }

    // I assume normalized paths are always smaller than non-normalized paths
    // if (temp.length > STDX_x_smallstr_MAX_LENGTH)
    //  return -1;

    input->length = temp.length;
    memcpy(input->buf, temp.buf, temp.length);
    input->buf[temp.length] = '\0';

    return input;
  }

  XStrview x_fs_path_basename(const int8_t* input)
  {
    XStrview empty = {0};
    if (!input)
      return empty;

    const int8_t* last_sep = find_last_path_separator(input);
    return x_strview(last_sep ? last_sep + 1 : input);
  }

  XStrview x_fs_path_dirname(const int8_t* input)
  {
    XStrview empty =
    {0};
    if (!input) return empty;

    size_t input_len = strlen(input);
    if (input_len == 0)
      return empty;

    // Find the last slash or backslash
    const int8_t* last_sep = find_last_path_separator(input);
    if (!last_sep)
      return empty;

    size_t len = (size_t)(last_sep - input);

    if (len == 0)

    {
      // Root case: "/file" → "/"
      int8_t root[2] =
      { *last_sep, '\0' };
      return (XStrview){.data = root, .length = 1 };
    }

    return (XStrview){.data = input, .length = len };
  }

  XStrview x_fs_path_extension(const int8_t* input)
  {
    const int8_t* dot = strrchr(input, '.');
    if (!dot || strrchr(input, PATH_SEPARATOR) > dot)
      return x_strview("");

    return x_strview(dot + 1);
  }

  size_t x_fs_path_change_extension(XFSPath* path, const int8_t* new_ext)
  {
    if (!path || !new_ext) return -1;

    const int8_t* path_str = x_smallstr_cstr(path);
    const int8_t* last_dot = NULL;
    const int8_t* last_sep = NULL;

    // Scan from the end to locate '.' and path separator
    for (ptrdiff_t i = (ptrdiff_t)(path->length - 1); i >= 0; --i)

    {
      int8_t c = path_str[i];
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
    if (new_ext[0] != '.')
    {
      if (x_smallstr_append_char(path, '.') <= 0)
        return -1;
    }

    return x_smallstr_append_cstr(path, new_ext);
  }

  bool x_fs_path_is_absolute_native_cstr(const int8_t* path)
  {
#ifdef _WIN32
    const int8_t* p = path;
    size_t length = strlen(path);
    if (length >= 3)
      return (isalpha(p[0]) && p[1] == ':' && (p[2] == '\\' || p[2] == '/')) || (p[0] == '\\' && p[1] == '\\');
    else
      return (isalpha(p[0]) && p[1] == ':') || (p[0] == '\\' && p[1] == '\\');
#else
    return path[0] == '/';
#endif
  }

  bool x_fs_path_is_absolute_native(const XFSPath* path)
  {
    return x_fs_path_is_absolute_native_cstr(x_fs_path_cstr(path));
  }

  static inline bool x_fs_path_eq_cstr_cstr(const int8_t* a, const int8_t* b)
  {
    if (!a || !b)
      return false;

    while (*a && *b)
    {
      int8_t ca = is_path_separator(*a) ? '/' : *a;
      int8_t cb = is_path_separator(*b) ? '/' : *b;
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

  bool x_fs_path_eq(const XFSPath* path_a, const XFSPath* path_b)
  {
    if (path_a->buf == NULL && path_b->buf == NULL)
      return false;

    if (path_a->length == 0 && path_b->length == 0)
      return false;

    const int8_t* a = &path_a->buf[0];
    const int8_t* b = &path_b->buf[0];
    return x_fs_path_eq_cstr_cstr(a, b);
  }

  bool x_fs_path_eq_cstr(const XFSPath* path_a, const int8_t* path_b)
  {
    if (path_a->buf == NULL && path_b == NULL)
      return false;

    const int8_t* a = &(path_a->buf[0]);
    return x_fs_path_eq_cstr_cstr(a, path_b);
  }

  bool x_fs_path_is_relative_cstr(const int8_t* path)
  {
    return !x_fs_path_is_absolute_cstr(path);
  }

  bool x_fs_path_is_absolute_cstr(const int8_t* path)
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

  bool x_fs_path_is_absolute(const XFSPath* path)
  {
    return x_fs_path_is_absolute_cstr(x_fs_path_cstr(path));
  }

  size_t x_fs_path_relative(const int8_t* from_path, const int8_t* to_path, XFSPath* out_path)
  {
    if (!from_path || !to_path || !out_path) return -1;

    XFSPath from_copy;
    x_fs_path_init(&from_copy);

    size_t from_len = strlen(from_path);
    if (from_len > STDX_SMALLSTR_MAX_LENGTH)
      from_len = STDX_SMALLSTR_MAX_LENGTH;

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
      if (strncmp((const int8_t*)from_copy.buf, to_path, prefix_len) == 0)
      {
        if (to_len == prefix_len)
        {
          // Paths are identical
          x_fs_path_init(out_path);
          x_smallstr_from_cstr(out_path, ".");
          return 0;
        }
        if (is_path_separator(to_path[prefix_len]))
        {
          // Return suffix after prefix + separator
          const int8_t* suffix = to_path + prefix_len + 1;
          x_fs_path_init(out_path);
          x_smallstr_from_cstr(out_path, suffix);
          return 0;
        }
      }
    }

    // No prefix match, return to_path as is
    return x_smallstr_from_cstr(out_path, to_path);
  }

  // Input must be normalized
  bool x_fs_path_split(const int8_t* input, XFSPath* out_components, size_t max_components, size_t* out_count)
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
          tmp.length = (part_len > STDX_SMALLSTR_MAX_LENGTH) ? STDX_SMALLSTR_MAX_LENGTH : part_len;
          strncpy((char*)tmp.buf, input + start, tmp.length);
          tmp.buf[tmp.length] = '\0';

          x_smallstr_from_cstr(&out_components[count], (const int8_t*)tmp.buf);
          count++;
        }
        start = i + 1;
      }
    }

    *out_count = count;
    return true;
  }

  int32_t x_fs_path_compare(const XFSPath* a, const XFSPath* b)
  {
    if (!a || !b) return -1;

    size_t alen = trim_trailing_separators(a->buf, a->length);
    size_t blen = trim_trailing_separators(b->buf, b->length);
    size_t min_len = (alen < blen) ? alen : blen;

    for (size_t i = 0; i < min_len; ++i)
    {
      int8_t ca = normalized_path_char(a->buf[i]);
      int8_t cb = normalized_path_char(b->buf[i]);
      if (ca != cb) return (uint8_t)ca - (uint8_t)cb;
    }

    return (int)(alen - blen);
  }

  int32_t x_fs_path_compare_cstr(const XFSPath* a, const int8_t* cstr)
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
      int8_t ca = normalized_path_char(a->buf[i]);
      int8_t cb = normalized_path_char(cstr[i]);
      if (ca != cb) return (uint8_t)ca - (uint8_t)cb;
    }

    return (int)(alen - clen);
  }

  bool x_fs_path_exists_cstr(const int8_t* path)
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

  bool x_fs_path_exists(const XFSPath* path)
  {
    return x_fs_path_exists_cstr(x_fs_path_cstr(path));
  }

  bool x_fs_path_exists_quick_cstr(const int8_t* path)
  {
#ifdef _WIN32
    DWORD attributes = GetFileAttributes(path);
    return attributes != INVALID_FILE_ATTRIBUTES;
#else
    return access(path, F_OK) == 0;
#endif
  }

  bool x_fs_path_exists_quick(const XFSPath* path)
  {
    return x_fs_path_exists_quick_cstr(x_fs_path_cstr(path));
  }

  size_t x_fs_path_from_strview(XStrview sv, XFSPath* out)
  {
    return x_smallstr_from_strview(sv, out);
  }

  bool x_fs_file_stat(const int8_t* path, FSFileStat* out_stat)
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

  bool x_fs_file_modification_time(const int8_t* path, time_t* out_time)
  {
    FSFileStat stat;
    if (x_fs_file_stat(path, &stat) != 0) return false;
    *out_time = stat.modification_time;
    return true;
  }

  bool x_fs_file_creation_time(const int8_t* path, time_t* out_time)
  {
    FSFileStat stat;
    if (x_fs_file_stat(path, &stat) != 0) return false;
    *out_time = stat.creation_time;
    return true;
  }

  bool x_fs_file_permissions(const int8_t* path, uint32_t* out_permissions)
  {
    FSFileStat stat;
    if (x_fs_file_stat(path, &stat) != 0) return false;
    *out_permissions = stat.permissions;
    return true;
  }

  bool x_fs_file_set_permissions(const int8_t* path, uint32_t permissions)
  {
#ifdef _WIN32
    return SetFileAttributesA(path, permissions);
#else
    return chmod(path, permissions) == 0;
#endif
  }

  bool x_fs_is_file(const int8_t* path)
  {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    return (stat(path, &st) == 0) && S_ISREG(st.st_mode);
#endif
  }

  bool x_fs_is_directory(const int8_t* path)
  {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    return (stat(path, &st) == 0) && S_ISDIR(st.st_mode);
#endif
  }

  bool x_fs_is_symlink(const int8_t* path)
  {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_REPARSE_POINT);
#else
    struct stat st;
    return (lstat(path, &st) == 0) && S_ISLNK(st.st_mode);
#endif
  }

  bool x_fs_read_symlink(const int8_t* path, XFSPath* out_path)
  {
#ifdef _WIN32
    // No direct equivalent in Win32 API; require fallback with GetFinalPathNameByHandle or similar
    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    int8_t buf[MAX_PATH];
    DWORD len = GetFinalPathNameByHandleA(hFile, buf, MAX_PATH, FILE_NAME_NORMALIZED);
    CloseHandle(hFile);
    if (len == 0 || len >= MAX_PATH) return false;
    return x_smallstr_from_cstr(out_path, buf);
#else
    int8_t buf[PATH_MAX];
    ssize_t len = readlink(path, buf, sizeof(buf) - 1);
    if (len == -1) return false;
    buf[len] = '\0';
    return x_smallstr_from_cstr(out_path, buf) > 0;
#endif
  }

  bool x_fs_make_temp_file(const int8_t* prefix, XFSPath* out_path)
  {
#ifdef _WIN32
    int8_t tmpPath[MAX_PATH];
    if (!GetTempPathA(MAX_PATH, tmpPath)) return false;

    int8_t tmpFile[MAX_PATH];
    if (!GetTempFileNameA(tmpPath, prefix, 0, tmpFile)) return false;
    return x_smallstr_from_cstr(out_path, tmpFile);
#else
    int8_t tmpl[PATH_MAX];
    snprintf(tmpl, sizeof(tmpl), "/tmp/%sXXXXXX", prefix);
    int32_t fd = mkstemp(tmpl);
    if (fd == -1) return false;
    close(fd);
    return x_smallstr_from_cstr(out_path, tmpl) > 0;
#endif
  }

  bool x_fs_make_temp_directory(const int8_t* prefix, XFSPath* out_path)
  {
#ifdef _WIN32
    int8_t tmpPath[MAX_PATH];
    if (!GetTempPathA(MAX_PATH, tmpPath)) return false;

    int8_t dirPath[MAX_PATH];
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
    int8_t tmpl[PATH_MAX];
    snprintf(tmpl, sizeof(tmpl), "/tmp/%sXXXXXX", prefix);
    if (!mkdtemp(tmpl)) return -1;
    return x_smallstr_from_cstr(out_path, tmpl) > 0;
#endif
  }

#endif  // STDX_IMPLEMENTATION_FILESYSTEM

#ifdef STDX_INTERNAL_STRING_IMPLEMENTATION
#undef STDX_IMPLEMENTATION_STRING
#undef STDX_INTERNAL_STRING_IMPLEMENTATION
#endif

#ifdef __cplusplus
}
#endif

#endif  // STDX_FILESYSTEM_H
