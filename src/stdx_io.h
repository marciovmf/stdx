/*
 * STDX - IO
 * Part of the STDX General Purpose C Library by marciovmf
 * https://github.com/marciovmf/stdx
 *
 * Provides file i/o utility functions
 *
 * To compile the implementation, define:
 *     #define X_IMPL_IO
 * in **one** source file before including this header.
 *
 * Author: marciovmf
 * License: MIT
 * Usage: #include "stdx_io.h"
 */
#ifndef X_IO_H
#define X_IO_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" 
{
#endif

#define X_IO_VERSION_MAJOR 1
#define X_IO_VERSION_MINOR 0
#define X_IO_VERSION_PATCH 0
#define X_IO_VERSION (X_IO_VERSION_MAJOR * 10000 + X_IO_VERSION_MINOR * 100 + X_IO_VERSION_PATCH)

  typedef struct XFile_t XFile;

  XFile *x_io_open(const char *filename, const char *mode);                         // Open a file with mode ("r", "rb", "w", etc.)
  void x_io_close(XFile *file);                                                     // Close a file and free internal memory
  size_t x_io_read(XFile *file, void *buffer, size_t size);                         // Read up to `size` bytes into buffer. Returns bytes read.
  char *x_io_read_all(XFile *file, size_t *out_size);                               // Read the entire file into a buffer. Returns buffer (null-terminated, but not for text safety). Caller must free.
  char *x_io_read_text(const char *filename, size_t* out_size);                     // Convenience: open, read, close. Returns null-terminated text.
  size_t x_io_write(XFile *file, const void *data, size_t size);                    // Write `size` bytes to file. Returns number of bytes written.
  bool x_io_write_text(const char *filename, const char *text);                     // Write null-terminated text to a file (overwrite).
  bool x_io_append_text(const char *filename, const char *text);                    // Append null-terminated text to file.
  bool x_io_seek(XFile *file, long offset, int32_t origin);                         // Seek within file.
  long x_io_tell(XFile *file);                                                      // Return current file position.
  bool x_io_rewind(XFile *file);                                                    // Rewind file to beginning.
  bool x_io_flush(XFile *file);                                                     // Flush file buffer.
  bool x_io_eof(XFile *file);                                                       // Check end-of-file.
  bool x_io_error(XFile *file);                                                     // Check for file error.
  void x_io_clearerr(XFile *file);                                                  // Clear file error and EOF flags.
  int32_t x_io_fileno(XFile *file);                                                 // Return underlying file descriptor.

#ifdef __cplusplus
}
#endif

#ifdef X_IMPL_IO
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef X_IO_ALLOC
#define X_IO_ALLOC(sz)        malloc(sz)
#define X_IO_FREE(p)          free(p)
#endif

#ifdef __cplusplus
extern "C" 
{
#endif

  struct XFile_t 
  {
    FILE *fp;
  };

  XFile *x_io_open(const char *filename, const char *mode) 
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

  void x_io_close(XFile *file) 
  {
    if (!file) return;
    fclose(file->fp);
    X_IO_FREE(file);
  }

  size_t x_io_read(XFile *file, void *buffer, size_t size) 
  {
    if (!file || !buffer || size == 0) return 0;
    return fread(buffer, 1, size, file->fp);
  }

  size_t x_io_write(XFile *file, const void *data, size_t size) 
  {
    if (!file || !data || size == 0) return 0;
    return fwrite(data, 1, size, file->fp);
  }

  char *x_io_read_all(XFile *file, size_t *out_size) 
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

  char *x_io_read_text(const char *filename, size_t* out_size) 
  {
    size_t size = 0;
    XFile *f = x_io_open(filename, "rb");
    if (!f) return NULL;
    char *text = x_io_read_all(f, out_size);
    x_io_close(f);
    return text;
  }

  bool x_io_write_text(const char *filename, const char *text) 
  {
    XFile *f = x_io_open(filename, "wb");
    if (!f) return false;
    size_t len = strlen(text);
    size_t written = x_io_write(f, text, len);
    x_io_close(f);
    return written == len;
  }

  bool x_io_append_text(const char *filename, const char *text) 
  {
    XFile *f = x_io_open(filename, "ab");
    if (!f) return false;
    size_t len = strlen(text);
    size_t written = x_io_write(f, text, len);
    x_io_close(f);
    return written == len;
  }

  bool x_io_seek(XFile *file, long offset, int32_t origin) 
  {
    return file && fseek(file->fp, offset, origin) == 0;
  }

  long x_io_tell(XFile *file) 
  {
    return file ? ftell(file->fp) : -1;
  }

  bool x_io_rewind(XFile *file) 
  {
    if (!file) return false;
    rewind(file->fp);
    return true;
  }

  bool x_io_flush(XFile *file) 
  {
    return file && fflush(file->fp) == 0;
  }

  bool x_io_eof(XFile *file) 
  {
    return file && feof(file->fp);
  }

  bool x_io_error(XFile *file) 
  {
    return file && ferror(file->fp);
  }

  void x_io_clearerr(XFile *file) 
  {
    if (file) clearerr(file->fp);
  }

  int32_t x_io_fileno(XFile *file) 
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
