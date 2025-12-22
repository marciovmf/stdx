/**
 * STDX - IO (file I/O utility functions)
 * Part of the STDX General Purpose C Library by marciovmf
 * <https://github.com/marciovmf/stdx>
 * License: MIT
 *
 * ## Overview
 * Provides basic file io operations. 
 *
 * To compile the implementation define `X_IMPL_IO`
 * in **one** source file before including this header.
 *
 * ## How to compile
 *
 * To customize how this module allocates memory, define
 * `X_IO_ALLOC` / `X_IO_FREE` before including.
 */
#ifndef X_IO_H
#define X_IO_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>


#define X_IO_VERSION_MAJOR 1
#define X_IO_VERSION_MINOR 0
#define X_IO_VERSION_PATCH 0
#define X_IO_VERSION (X_IO_VERSION_MAJOR * 10000 + X_IO_VERSION_MINOR * 100 + X_IO_VERSION_PATCH)

#ifndef X_IO_API
#define X_IO_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

  typedef struct XFile_t XFile;

  /**
   * @brief Open a file with the specified mode.
   * @param filename Path to the file to open.
   * @param mode Standard C file mode string (e.g. "r", "rb", "w", "a").
   * @return Pointer to an XFile on success, or NULL on failure.
   */
  X_IO_API XFile *x_io_open(const char *filename, const char *mode);

  /**
   * @brief Close a file and release all associated resources.
   * @param file File handle to close.
   */
  X_IO_API void x_io_close(XFile *file);

  /**
   * @brief Read raw bytes from a file.
   * @param file File handle to read from.
   * @param buffer Destination buffer.
   * @param size Maximum number of bytes to read.
   * @return Number of bytes actually read.
   */
  X_IO_API size_t x_io_read(XFile *file, void *buffer, size_t size);

  /**
   * @brief Read the entire contents of a file into memory.
   * @param file Open file handle.
   * @param out_size Optional pointer to receive file size in bytes.
   * @return Heap-allocated buffer containing file data, null-terminated.
   */
  X_IO_API char *x_io_read_all(XFile *file, size_t *out_size);

  /**
   * @brief Read an entire text file into memory.
   * @param filename Path to the file to read.
   * @param out_size Optional pointer to receive text size in bytes.
   * @return Heap-allocated, null-terminated text buffer, or NULL on failure.
   */
  X_IO_API char *x_io_read_text(const char *filename, size_t *out_size);

  /**
   * @brief Write raw bytes to a file.
   * @param file File handle to write to.
   * @param data Source buffer.
   * @param size Number of bytes to write.
   * @return Number of bytes actually written.
   */
  X_IO_API size_t x_io_write(XFile *file, const void *data, size_t size);

  /**
   * @brief Write a null-terminated text string to a file, overwriting it.
   * @param filename Path to the file.
   * @param text Null-terminated text to write.
   * @return true on success, false on failure.
   */
  X_IO_API bool x_io_write_text(const char *filename, const char *text);

  /**
   * @brief Append a null-terminated text string to a file.
   * @param filename Path to the file.
   * @param text Null-terminated text to append.
   * @return true on success, false on failure.
   */
  X_IO_API bool x_io_append_text(const char *filename, const char *text);

  /**
   * @brief Change the current file position.
   * @param file File handle.
   * @param offset Offset relative to origin.
   * @param origin Seek origin (e.g. SEEK_SET, SEEK_CUR, SEEK_END).
   * @return true on success, false on failure.
   */
  X_IO_API bool x_io_seek(XFile *file, long offset, int32_t origin);

  /**
   * @brief Get the current file position.
   * @param file File handle.
   * @return Current position in bytes, or -1 on error.
   */
  X_IO_API long x_io_tell(XFile *file);

  /**
   * @brief Rewind the file position to the beginning.
   * @param file File handle.
   * @return true on success, false on failure.
   */
  X_IO_API bool x_io_rewind(XFile *file);

  /**
   * @brief Flush any buffered output to the file.
   * @param file File handle.
   * @return true on success, false on failure.
   */
  X_IO_API bool x_io_flush(XFile *file);

  /**
   * @brief Check whether end-of-file has been reached.
   * @param file File handle.
   * @return true if EOF is set, false otherwise.
   */
  X_IO_API bool x_io_eof(XFile *file);

  /**
   * @brief Check whether an error has occurred on the file.
   * @param file File handle.
   * @return true if an error is set, false otherwise.
   */
  X_IO_API bool x_io_error(XFile *file);

  /**
   * @brief Clear error and EOF indicators on a file.
   * @param file File handle.
   */
  X_IO_API void x_io_clearerr(XFile *file);

  /**
   * @brief Retrieve the underlying file descriptor.
   * @param file File handle.
   * @return Platform file descriptor, or -1 if unavailable.
   */
  X_IO_API int32_t x_io_fileno(XFile *file);

#ifdef __cplusplus
}
#endif

#ifdef X_IMPL_IO
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef X_IO_ALLOC
/**
 * @brief Internal macro for allocating memory.
 * To override how this header allocates memory, define this macro with a
 * different implementation before including this header.
 * @param sz  The size of memory to alloc.
 */
#define X_IO_ALLOC(sz)        malloc(sz)
#endif

#ifndef X_IO_FREE
/**
 * @brief Internal macro for freeing memory.
 * To override how this header frees memory, define this macro with a
 * different implementation before including this header.
 * @param p  The address of memory region to free.
 */
#define X_IO_FREE(p)          free(p)
#endif

#ifdef __cplusplus
extern "C" {
#endif

  struct XFile_t 
  {
    FILE *fp;
  };

  X_IO_API XFile *x_io_open(const char *filename, const char *mode) 
  {
    FILE *fp = fopen(filename, mode);
    if (!fp) return NULL;

    XFile *xf = (XFile *)X_IO_ALLOC(sizeof(XFile));
    if (!xf) 
    {
      fclose(fp);
      return NULL;
    }

    xf->fp = fp;
    return xf;
  }

  X_IO_API void x_io_close(XFile *file) 
  {
    if (!file) return;
    fclose(file->fp);
    X_IO_FREE(file);
  }

  X_IO_API size_t x_io_read(XFile *file, void *buffer, size_t size) 
  {
    if (!file || !buffer || size == 0) return 0;
    return fread(buffer, 1, size, file->fp);
  }

  X_IO_API size_t x_io_write(XFile *file, const void *data, size_t size) 
  {
    if (!file || !data || size == 0) return 0;
    return fwrite(data, 1, size, file->fp);
  }

  X_IO_API char *x_io_read_all(XFile *file, size_t *out_size) 
  {
    if (!file) return NULL;

    if (!x_io_seek(file, 0, SEEK_END)) return NULL;
    long len = x_io_tell(file);
    if (len < 0) return NULL;

    if (!x_io_rewind(file)) return NULL;

    char *buf = (char *)X_IO_ALLOC((size_t)len + 1);
    if (!buf) return NULL;

    size_t read = x_io_read(file, buf, (size_t)len);
    buf[read] = '\0';

    if (out_size) *out_size = read;
    return buf;
  }

  X_IO_API char *x_io_read_text(const char *filename, size_t* out_size) 
  {
    XFile *f = x_io_open(filename, "rb");
    if (!f) return NULL;
    char *text = x_io_read_all(f, out_size);
    x_io_close(f);
    return text;
  }

  X_IO_API bool x_io_write_text(const char *filename, const char *text) 
  {
    XFile *f = x_io_open(filename, "wb");
    if (!f) return false;
    size_t len = strlen(text);
    size_t written = x_io_write(f, text, len);
    x_io_close(f);
    return written == len;
  }

  X_IO_API bool x_io_append_text(const char *filename, const char *text) 
  {
    XFile *f = x_io_open(filename, "ab");
    if (!f) return false;
    size_t len = strlen(text);
    size_t written = x_io_write(f, text, len);
    x_io_close(f);
    return written == len;
  }

  X_IO_API bool x_io_seek(XFile *file, long offset, int32_t origin) 
  {
    return file && fseek(file->fp, offset, origin) == 0;
  }

  X_IO_API long x_io_tell(XFile *file) 
  {
    return file ? ftell(file->fp) : -1;
  }

  X_IO_API bool x_io_rewind(XFile *file) 
  {
    if (!file) return false;
    rewind(file->fp);
    return true;
  }

  X_IO_API bool x_io_flush(XFile *file) 
  {
    return file && fflush(file->fp) == 0;
  }

  X_IO_API bool x_io_eof(XFile *file) 
  {
    return file && feof(file->fp);
  }

  X_IO_API bool x_io_error(XFile *file) 
  {
    return file && ferror(file->fp);
  }

  X_IO_API void x_io_clearerr(XFile *file) 
  {
    if (file) clearerr(file->fp);
  }

  X_IO_API int32_t x_io_fileno(XFile *file) 
  {
    if (!file) return -1;
#if defined(_MSC_VER)
    return _fileno(file->fp);
#else
    return fileno(file->fp);
#endif
  }

#ifdef __cplusplus
}
#endif

#endif // X_IMPL_IO
#endif // X_IO_H
