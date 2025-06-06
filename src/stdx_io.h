/*
 * STDX - IO
 * Part of the STDX General Purpose C Library by marciovmf
 * https://github.com/marciovmf/stdx
 *
 * Provides a custom memory allocator interface.
 *
 * To compile the implementation, define:
 *     #define STDX_IMPLEMENTATION_IO
 * in **one** source file before including this header.
 *
 * Author: marciovmf
 * License: MIT
 * Usage: #include "stdx_io.h"
 */
#ifndef STDX_IO_H
#define STDX_IO_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" 
{
#endif

#define STDX_IO_VERSION_MAJOR 1
#define STDX_IO_VERSION_MINOR 0
#define STDX_IO_VERSION_PATCH 0

#define STDX_IO_VERSION (STDX_IO_VERSION_MAJOR * 10000 + STDX_IO_VERSION_MINOR * 100 + STDX_IO_VERSION_PATCH)

#ifdef STDX_IMPLEMENTATION_IO
#ifndef STDX_IMPLEMENTATION_ALLOCATOR
#define STDX_INTERNAL_ALLOCATOR_IMPLEMENTATION
#define STDX_IMPLEMENTATION_ALLOCATOR
#endif
#endif
#include <stdx_allocator.h>

  typedef struct XFile_t XFile;

  XFile *x_io_open(const char *filename, const char *mode, XAllocator *alloc);// Open a file with mode ("r", "rb", "w", etc.)
  void x_io_close(XFile *file); // Close a file and free internal memory
  size_t x_io_read(XFile *file, void *buffer, size_t size); // Read up to `size` bytes into buffer. Returns bytes read.
  char *x_io_read_all(XFile *file, size_t *out_size, XAllocator *alloc); // Read the entire file into a buffer allocated with `alloc` (or malloc). Returns buffer (null-terminated, but not for text safety). Caller must free.
  char *x_io_read_text(const char *filename, size_t* out_size, XAllocator *alloc); // Convenience: open, read, close. Returns null-terminated text.
  size_t x_io_write(XFile *file, const void *data, size_t size); // Write `size` bytes to file. Returns number of bytes written.
  bool x_io_write_text(const char *filename, const char *text); // Write null-terminated text to a file (overwrite).
  bool x_io_append_text(const char *filename, const char *text); // Append null-terminated text to file.
  bool x_io_seek(XFile *file, long offset, int32_t origin); // Seek within file.
  long x_io_tell(XFile *file); // Return current file position.
  bool x_io_rewind(XFile *file); // Rewind file to beginning.
  bool x_io_flush(XFile *file); // Flush file buffer.
  bool x_io_eof(XFile *file); // Check end-of-file.
  bool x_io_error(XFile *file); // Check for file error.
  void x_io_clearerr(XFile *file); // Clear file error and EOF flags.
  int32_t x_io_fileno(XFile *file); // Return underlying file descriptor.

#ifdef STDX_IMPLEMENTATION_IO

#include "stdx_io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

  struct XFile_t 
  {
    FILE *fp;
    XAllocator *alloc;
  };

  static void *x_io_alloc(XAllocator *a, size_t size) 
  {
    return stdx_alloc(a, size);
  }

  static void x_io_free(XAllocator *a, void *ptr) 
  {
    if (!ptr) return;
    if (a) a->free(a, ptr);
    else free(ptr);
  }

  XFile *x_io_open(const char *filename, const char *mode, XAllocator *alloc) 
  {
    FILE *fp = fopen(filename, mode);
    if (!fp) return NULL;

    XFile *xf = (XFile *)x_io_alloc(alloc, sizeof(XFile));
    if (!xf) 
    {
      fclose(fp);
      return NULL;
    }

    xf->fp = fp;
    xf->alloc = alloc;
    return xf;
  }

  void x_io_close(XFile *file) 
  {
    if (!file) return;
    fclose(file->fp);
    x_io_free(file->alloc, file);
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

  char *x_io_read_all(XFile *file, size_t *out_size, XAllocator *alloc) 
  {
    if (!file) return NULL;

    if (!x_io_seek(file, 0, SEEK_END)) return NULL;
    long len = x_io_tell(file);
    if (len < 0) return NULL;

    if (!x_io_rewind(file)) return NULL;

    char *buf = (char *)x_io_alloc(alloc, (size_t)len + 1);
    if (!buf) return NULL;

    size_t read = x_io_read(file, buf, (size_t)len);
    buf[read] = '\0';

    if (out_size) *out_size = read;
    return buf;
  }

  char *x_io_read_text(const char *filename, size_t* out_size, XAllocator *alloc) 
  {
    size_t size = 0;
    XFile *f = x_io_open(filename, "rb", alloc);
    if (!f) return NULL;
    char *text = x_io_read_all(f, out_size, alloc);
    x_io_close(f);
    return text;
  }

  bool x_io_write_text(const char *filename, const char *text) 
  {
    XFile *f = x_io_open(filename, "wb", NULL);
    if (!f) return false;
    size_t len = strlen(text);
    size_t written = x_io_write(f, text, len);
    x_io_close(f);
    return written == len;
  }

  bool x_io_append_text(const char *filename, const char *text) 
  {
    XFile *f = x_io_open(filename, "ab", NULL);
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

#endif // STDX_IMPLEMENTATION_IO

#ifdef STDX_INTERNAL_ALLOCATOR_IMPLEMENTATION
#undef STDX_IMPLEMENTATION_ALLOCATOR
#undef STDX_INTERNAL_ALLOCATOR_IMPLEMENTATION
#endif

#ifdef __cplusplus
}
#endif

#endif // STDX_IO_H
