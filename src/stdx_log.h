/**
 * STDX - Logging Utilities
 * Part of the STDX General Purpose C Library by marciovmf
 * <https://github.com/marciovmf/stdx>
 * License: MIT
 *
 * ## Overview
 *
 * Provides a flexible logging system with support for:
 * - Logger initialization and cleanup
 * - Log levels with color-coded output
 * - Source location tagging (file, line, function)
 * - Multiple log output targets selectable via flags
 * - Convenience macros for common log levels (debug, info, warning, error, fatal)
 *
 * ## How to compile
 *
 * To compile the implementation define `X_IMPL_LOG`
 * in **one** source file before including this header.
 */

#ifndef X_LOG_H
#define X_LOG_H

#define X_LOG_VERSION_MAJOR 1
#define X_LOG_VERSION_MINOR 0
#define X_LOG_VERSION_PATCH 0
#define X_LOG_VERSION (X_LOG_VERSION_MAJOR * 10000 + X_LOG_VERSION_MINOR * 100 + X_LOG_VERSION_PATCH)

#ifndef X_LOG_BUFFER_SIZE
/**
 * Default buffer size for log messages.
 * Can be overriden before including this header
 */
#define X_LOG_BUFFER_SIZE (1024 * 4)
#endif  // X_LOG_BUFFER_SIZE

#ifdef _WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

  typedef enum
  {
    XLOG_LEVEL_DEBUG = 0,
    XLOG_LEVEL_INFO,
    XLOG_LEVEL_WARNING,
    XLOG_LEVEL_ERROR,
    XLOG_LEVEL_FATAL,
  } XLogLevel;

  typedef enum
  {
    XLOG_OUTPUT_NONE       = 0,
    XLOG_OUTPUT_CONSOLE    = 1 << 0,
    XLOG_OUTPUT_FILE       = 1 << 1,
    XLOG_OUTPUT_BOTH       = XLOG_OUTPUT_CONSOLE | XLOG_OUTPUT_FILE,
  } XLogOutputFlags;

  typedef enum
  {
    XLOG_PLAIN      = 0,
    XLOG_TIMESTAMP  = 1 << 0,
    XLOG_TAG        = 1 << 1,
    XLOG_SOURCEINFO = 1 << 2,
    XLOG_DEFAULT    = XLOG_TAG | XLOG_TIMESTAMP | XLOG_SOURCEINFO
  } XLogComponent;

  typedef struct
  {
    FILE* console;           /* Console output stream (stdout/stderr). Defaults to stdout. */
    FILE* file;              /* Log file pointer (optional) */
    bool file_owned;         /* True if logger opened the file and should close it */
    int outputs;             /* Which outputs enabled (console/file/both) */
    XLogLevel level;         /* Minimum level to log */
#ifdef _WIN32
    bool vt_enabled;         /* Windows VT ANSI mode enabled? */
#endif
  } XLogger;

  typedef enum
  {
    XLOG_COLOR_DEFAULT,
    XLOG_COLOR_BLACK,
    XLOG_COLOR_RED,
    XLOG_COLOR_GREEN,
    XLOG_COLOR_YELLOW,
    XLOG_COLOR_BLUE,
    XLOG_COLOR_MAGENTA,
    XLOG_COLOR_CYAN,
    XLOG_COLOR_WHITE,
    XLOG_COLOR_BRIGHT_BLACK,
    XLOG_COLOR_BRIGHT_RED,
    XLOG_COLOR_BRIGHT_GREEN,
    XLOG_COLOR_BRIGHT_YELLOW,
    XLOG_COLOR_BRIGHT_BLUE,
    XLOG_COLOR_BRIGHT_MAGENTA,
    XLOG_COLOR_BRIGHT_CYAN,
    XLOG_COLOR_BRIGHT_WHITE,
  } XLogColor;

  /**
   * @brief Initialize the logging system.
   * @param outputs Bitmask specifying enabled log outputs (console, file, etc.).
   * @param level Minimum log level to emit.
   * @param filename Optional file path for file output, or NULL if unused.
   */
  void logger_init(XLogOutputFlags outputs, XLogLevel level, const char *filename);

  /**
   * @brief Shutdown the logging system and release resources.
   */
  void logger_close(void);

  /**
   * @brief Set the output stream used for console logging.
   *
   * This does not affect file logging configured via logger_init() (XLOG_OUTPUT_FILE).
   * Passing NULL resets to stdout.
   */
  void logger_set_console(FILE* out);

  /**
   * @brief Get the current console output stream (never NULL; defaults to stdout).
   */
  FILE* logger_get_console(void);

  /**
   * @brief Emit a formatted log message with full context information.
   * @param level Log severity level.
   * @param fg Foreground color.
   * @param bg Background color.
   * @param components Bitmask controlling which components (timestamp, file, line, etc.) are included.
   * @param file Source file name.
   * @param line Source line number.
   * @param func Source function name.
   * @param fmt printf-style format string.
   */
  void logger_log(
      XLogLevel level,
      XLogColor fg,
      XLogColor bg,
      XLogComponent components,
      const char* file,
      int line,
      const char* func,
      const char* fmt,
      ...
      );

  /**
   * @brief Emit a formatted log message with full context information, overriding the console output stream.
   *
   * If console output is enabled (XLOG_OUTPUT_CONSOLE), the message is written to `out` when non-NULL.
   * If `out` is NULL, the logger's default console stream is used.
   * File output configured via logger_init() is still honored when XLOG_OUTPUT_FILE is enabled.
   */
  void logger_log_to(
      FILE* out,
      XLogLevel level,
      XLogColor fg,
      XLogColor bg,
      XLogComponent components,
      const char* file,
      int line,
      const char* func,
      const char* fmt,
      ...
      );

  /**
   * @brief Emit a formatted log message without source context.
   * @param level Log severity level.
   * @param fmt printf-style format string.
   */
  void logger_print(XLogLevel level, const char* fmt, ...);

  /**
   * @brief Platform- or user-defined break action for fatal logs.
   *
   * This macro can be overridden to trigger a debugger break or abort.
   * Default implementation does nothing.
   */
#ifndef X_LOG_BREAK
#define X_LOG_BREAK() ((void)0)
#endif

  /**
   * @brief Emit a raw log message with explicit formatting and components.
   *
   * Automatically injects source file, line, and function information.
   */
#define x_log_raw(out, level, fg, bg, components, fmt, ...) \
  logger_log_to(out, level, fg, bg, components, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

  /**
   * @brief Emit a debug-level log message.
   */
#define x_log_debug(fmt, ...) \
  logger_log(XLOG_LEVEL_DEBUG, XLOG_COLOR_BLUE, XLOG_COLOR_BLACK, XLOG_DEFAULT, __FILE__, __LINE__, __func__, (const char*)fmt "\n", ##__VA_ARGS__)

  /**
   * @brief Emit an informational log message with timestamp.
   */
#define x_log_info(fmt, ...) \
  logger_log(XLOG_LEVEL_INFO, XLOG_COLOR_WHITE, XLOG_COLOR_BLACK, XLOG_TIMESTAMP, __FILE__, __LINE__, __func__, (const char*)fmt "\n", ##__VA_ARGS__)

  /**
   * @brief Emit a warning-level log message.
   */
#define x_log_warning(fmt, ...) \
  logger_log(XLOG_LEVEL_WARNING, XLOG_COLOR_YELLOW, XLOG_COLOR_BLACK, XLOG_DEFAULT, __FILE__, __LINE__, __func__, (const char*)fmt "\n", ##__VA_ARGS__)

  /**
   * @brief Emit an error-level log message.
   */
#define x_log_error(fmt, ...) \
  logger_log(XLOG_LEVEL_ERROR, XLOG_COLOR_RED, XLOG_COLOR_BLACK, XLOG_DEFAULT, __FILE__, __LINE__, __func__, (const char*)fmt "\n", ##__VA_ARGS__)

  /**
   * @brief Emit a fatal log message and trigger a break action.
   *
   * Calls X_LOG_BREAK() after logging.
   */
#define x_log_fatal(fmt, ...) \
  do { \
    logger_log(XLOG_LEVEL_FATAL, XLOG_COLOR_WHITE, XLOG_COLOR_RED, XLOG_DEFAULT, __FILE__, __LINE__, __func__, (const char*)fmt "\n", ##__VA_ARGS__); \
    X_LOG_BREAK(); \
  } while (0)


#ifdef __cplusplus
}
#endif

#ifdef X_IMPL_LOG

#ifndef _CRT_SECURE_NO_WARNINGS 
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif 
#include <windows.h>
#endif /* _WIN32 */

#ifdef __cplusplus
extern "C" {
#endif

  static XLogger g_logger =
  {
    .console = NULL,
    .file = NULL,
    .file_owned = false,
    .outputs = XLOG_OUTPUT_CONSOLE,
    .level = XLOG_LEVEL_DEBUG,
#ifdef _WIN32
    .vt_enabled = false,
#endif
  };

  /* Internal helpers */

  static int map_color_to_ansi(XLogColor color, bool fg)
  {
    const int base = fg ? 30 : 40;

    switch (color)
    {
      case XLOG_COLOR_BLACK:          return base + 0;
      case XLOG_COLOR_RED:            return base + 1;
      case XLOG_COLOR_GREEN:          return base + 2;
      case XLOG_COLOR_YELLOW:         return base + 3;
      case XLOG_COLOR_BLUE:           return base + 4;
      case XLOG_COLOR_MAGENTA:        return base + 5;
      case XLOG_COLOR_CYAN:           return base + 6;
      case XLOG_COLOR_WHITE:          return base + 7;

      /* Bright colors map to 90-97 (fg) and 100-107 (bg). */
      case XLOG_COLOR_BRIGHT_BLACK:   return (fg ? 90 : 100) + 0;
      case XLOG_COLOR_BRIGHT_RED:     return (fg ? 90 : 100) + 1;
      case XLOG_COLOR_BRIGHT_GREEN:   return (fg ? 90 : 100) + 2;
      case XLOG_COLOR_BRIGHT_YELLOW:  return (fg ? 90 : 100) + 3;
      case XLOG_COLOR_BRIGHT_BLUE:    return (fg ? 90 : 100) + 4;
      case XLOG_COLOR_BRIGHT_MAGENTA: return (fg ? 90 : 100) + 5;
      case XLOG_COLOR_BRIGHT_CYAN:    return (fg ? 90 : 100) + 6;
      case XLOG_COLOR_BRIGHT_WHITE:   return (fg ? 90 : 100) + 7;

      default:                        return fg ? 39 : 49; /* default fg/bg */
    }
  }

#ifdef _WIN32

  /* Enable VT processing on Windows 10+ */
  static inline void enable_windows_vt(void)
  {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (SetConsoleMode(hOut, dwMode))
    {
      g_logger.vt_enabled = true;
    }
  }

  /* Windows console API color codes for fallback */
  static inline WORD win_console_color(XLogLevel level)
  {
    switch (level)
    {
      case XLOG_LEVEL_DEBUG: return FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
      case XLOG_LEVEL_INFO: return 15 | FOREGROUND_INTENSITY;
      case XLOG_LEVEL_WARNING: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
      case XLOG_LEVEL_ERROR: return FOREGROUND_RED | FOREGROUND_INTENSITY;
      case XLOG_LEVEL_FATAL: return BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_INTENSITY;
      default: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }
  }

  static inline WORD map_color_to_win_attr(XLogColor color, bool is_fg)
  {
    switch (color)
    {
      case XLOG_COLOR_BLACK:          return is_fg ? 0 : 0;
      case XLOG_COLOR_RED:            return is_fg ? FOREGROUND_RED : BACKGROUND_RED;
      case XLOG_COLOR_GREEN:          return is_fg ? FOREGROUND_GREEN : BACKGROUND_GREEN;
      case XLOG_COLOR_YELLOW:         return is_fg ? FOREGROUND_RED | FOREGROUND_GREEN : BACKGROUND_RED | BACKGROUND_GREEN;
      case XLOG_COLOR_BLUE:           return is_fg ? FOREGROUND_BLUE : BACKGROUND_BLUE;
      case XLOG_COLOR_MAGENTA:        return is_fg ? FOREGROUND_RED | FOREGROUND_BLUE : BACKGROUND_RED | BACKGROUND_BLUE;
      case XLOG_COLOR_CYAN:           return is_fg ? FOREGROUND_GREEN | FOREGROUND_BLUE : BACKGROUND_GREEN | BACKGROUND_BLUE;
      case XLOG_COLOR_WHITE:          return is_fg ? FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE : BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
      case XLOG_COLOR_BRIGHT_BLACK:   return is_fg ? FOREGROUND_INTENSITY : 0;
      case XLOG_COLOR_BRIGHT_RED:     return is_fg ? FOREGROUND_RED | FOREGROUND_INTENSITY : BACKGROUND_RED | BACKGROUND_INTENSITY;
      case XLOG_COLOR_BRIGHT_GREEN:   return is_fg ? FOREGROUND_GREEN | FOREGROUND_INTENSITY : BACKGROUND_GREEN | BACKGROUND_INTENSITY;
      case XLOG_COLOR_BRIGHT_YELLOW:  return is_fg ? FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY : BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY;
      case XLOG_COLOR_BRIGHT_BLUE:    return is_fg ? FOREGROUND_BLUE | FOREGROUND_INTENSITY : BACKGROUND_BLUE | BACKGROUND_INTENSITY;
      case XLOG_COLOR_BRIGHT_MAGENTA: return is_fg ? FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY : BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_INTENSITY;
      case XLOG_COLOR_BRIGHT_CYAN:    return is_fg ? FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY : BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY;
      case XLOG_COLOR_BRIGHT_WHITE:   return is_fg ? FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY
                                      : BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY;
      default: return 0;
    }
  }

  /* Output message with Windows Console API colors */
  static inline void x_log_output_console_winapi(FILE* out, XLogColor fg, XLogColor bg, const char* msg)
  {
    HANDLE hConsole = GetStdHandle((out == stderr) ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE)
    {
      fputs(msg, out);
      return;
    }
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
    {
      fputs(msg, out);
      return;
    }
    WORD oldAttrs = csbi.wAttributes;
    WORD color = map_color_to_win_attr(fg, true)|map_color_to_win_attr(bg, false);
    SetConsoleTextAttribute(hConsole, color);
    fputs(msg, out);
    SetConsoleTextAttribute(hConsole, oldAttrs);
  }
#endif /* _WIN32 */

#ifndef _WIN32

  /* Output message with ANSI colors */
  static inline void x_log_output_console_ansi(FILE* out, XLogColor fg, XLogColor bg, const char* msg)
  {
    char color[32];
    snprintf(color, sizeof(color),
        "\x1b[%d;%dm",
        map_color_to_ansi(fg, true),
        map_color_to_ansi(bg, false));
    fprintf(out, "%s%s\x1b[0m", color, msg);
  }
#endif /* !_WIN32 */

  /* Common console output */
  static inline void x_log_output_console(FILE* out, XLogColor fg, XLogColor bg, const char* msg)
  {
#ifdef _WIN32
    //const char* color_code = ansi_color_code(level);


    if (g_logger.vt_enabled)
    {
      /* Use ANSI */
      fprintf(out,
          "\x1b[%d;%dm%s\x1b[0m",
          map_color_to_ansi(fg, true),
          map_color_to_ansi(bg, false),
          msg);
    }
    else
    {
      /* Use Windows Console API */
      x_log_output_console_winapi(out, fg, bg, msg);
    }
#else
    x_log_output_console_ansi(out, fg, bg, msg);
#endif
  }

  /* File output (no colors) */
  static inline void x_log_output_file(const char *msg)
  {
    if (g_logger.file)
    {
      fputs(msg, g_logger.file);
      fflush(g_logger.file);
    }
  }

  void logger_set_console(FILE* out)
  {
    g_logger.console = (out != NULL) ? out : stdout;
  }

  FILE* logger_get_console(void)
  {
    return (g_logger.console != NULL) ? g_logger.console : stdout;
  }

  static void s_x_logger_vlog_to(FILE* out, XLogLevel level, XLogColor fg, XLogColor bg, XLogComponent components, const char* file, int line, const char* func, const char* fmt, va_list args)
  {
    static const char* x_log_level_strings[] =
    {
      "DEBUG",
      "INFO",
      "WARNING",
      "ERROR",
      "FATAL"
    };

    if (level < g_logger.level)
    {
      return;
    }

    char timebuf[30] = {0};
    if (components & XLOG_TIMESTAMP)
    {
      time_t t = time(NULL);
      struct tm tm_info;

#ifdef _WIN32
      localtime_s(&tm_info, &t);
#else
      localtime_r(&t, &tm_info);
#endif
      strftime(timebuf, sizeof(timebuf), "[%Y-%m-%d %H:%M:%S] ", &tm_info);
    }

    char tag[32] = {0};
    if (components & XLOG_TAG)
    {
      snprintf((char*) tag, sizeof(tag), "%s ", x_log_level_strings[level]);
    }

    char source_info[1024] = {0};
    if (components & XLOG_SOURCEINFO)
    {
      snprintf(source_info, sizeof(source_info), "%s:%d %s() : ", file, line, func);
    }

    /* Format the message body */
    char msgbuf[X_LOG_BUFFER_SIZE];

#ifdef _WIN32
    vsnprintf_s(msgbuf, sizeof(msgbuf), _TRUNCATE, fmt, args);
#else
    vsnprintf(msgbuf, sizeof(msgbuf), fmt, args);
#endif

    char finalbuf[1280];
    snprintf(finalbuf, sizeof(finalbuf), "%s%s%s%s",
        tag,
        timebuf,
        source_info, msgbuf);

    if (g_logger.outputs & XLOG_OUTPUT_CONSOLE)
    {
      FILE* console_out = (out != NULL) ? out : logger_get_console();
      x_log_output_console(console_out, fg, bg, finalbuf);
    }

    if (g_logger.outputs & XLOG_OUTPUT_FILE)
    {
      x_log_output_file(finalbuf);
    }
  }

  void logger_log_to(FILE* out, XLogLevel level, XLogColor fg, XLogColor bg, XLogComponent components, const char* file, int line, const char* func, const char* fmt, ...)
  {
    va_list args;
    va_start(args, fmt);
    s_x_logger_vlog_to(out, level, fg, bg, components, file, line, func, fmt, args);
    va_end(args);
  }

  void logger_log(XLogLevel level, XLogColor fg, XLogColor bg, XLogComponent components, const char* file, int line, const char* func, const char* fmt, ...)
  {
    va_list args;
    va_start(args, fmt);
    s_x_logger_vlog_to(NULL, level, fg, bg, components, file, line, func, fmt, args);
    va_end(args);
  }

  /* Initialize logger */
  void logger_init(XLogOutputFlags outputs, XLogLevel level, const char *filename)
  {
    g_logger.outputs = outputs;
    g_logger.level = level;
    if (g_logger.console == NULL)
    {
      g_logger.console = stdout;
    }
    printf("log initialized\n");

#ifdef _WIN32
    enable_windows_vt();
#endif

    if ((outputs & XLOG_OUTPUT_FILE) && filename != NULL)
    {
      g_logger.file = fopen(filename, "a");
      g_logger.file_owned = (g_logger.file != NULL);
      if (!g_logger.file)
      {
        fprintf(stderr, "ERROR: Failed to open log file '%s'\n", filename);
        g_logger.file_owned = false;
        g_logger.outputs &= ~XLOG_OUTPUT_FILE; /* disable file output */
      }
    }
  }

  /* Close logger and free resources */
  void logger_close(void)
  {
    if (g_logger.file && g_logger.file_owned)
    {
      fclose(g_logger.file);
    }
    g_logger.file = NULL;
    g_logger.file_owned = false;
  }

#ifdef __cplusplus
}
#endif

#endif // X_IMPL_LOG
#endif // X_LOG_H

