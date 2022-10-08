#ifndef SANIC_LOG_H
#define SANIC_LOG_H

#include <stdio.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

enum sanic_log_level_enum {
    LEVEL_TRACE,
    LEVEL_INFO,
    LEVEL_WARN,
    LEVEL_ERROR
};

enum sanic_log_level_enum sanic_log_level;

#define sanic_log_trace(str) if(sanic_log_level <= LEVEL_TRACE) { printf(ANSI_COLOR_CYAN "[TRACE] "  str ANSI_COLOR_RESET "\n"); }
#define sanic_fmt_log_trace(str, ...) if(sanic_log_level <= LEVEL_TRACE) { printf(ANSI_COLOR_CYAN "[TRACE] "  str ANSI_COLOR_RESET "\n", __VA_ARGS__); }

#define sanic_log_info(str) if(sanic_log_level <= LEVEL_INFO) { printf(ANSI_COLOR_BLUE "[INFO] "  str ANSI_COLOR_RESET "\n"); }
#define sanic_fmt_log_info(str, ...) if(sanic_log_level <= LEVEL_INFO) { printf(ANSI_COLOR_BLUE "[INFO] "  str ANSI_COLOR_RESET "\n", __VA_ARGS__); }

#define sanic_log_warn(str) if(sanic_log_level <= LEVEL_WARN) { printf(ANSI_COLOR_YELLOW "[WARN] "  str ANSI_COLOR_RESET "\n"); }
#define sanic_fmt_log_warn(str, ...) if(sanic_log_level <= LEVEL_WARN) { printf(ANSI_COLOR_YELLOW "[WARN] "  str ANSI_COLOR_RESET "\n", __VA_ARGS__); }

#define sanic_log_error(str) if(sanic_log_level <= LEVEL_ERROR) { fprintf(stderr, ANSI_COLOR_RED "[ERROR] "  str ANSI_COLOR_RESET "\n"); }
#define sanic_fmt_log_error(str, ...) if(sanic_log_level <= LEVEL_ERROR) { fprintf(stderr, ANSI_COLOR_RED "[ERROR] "  str ANSI_COLOR_RESET "\n", __VA_ARGS__); }

#endif //SANIC_LOG_H
