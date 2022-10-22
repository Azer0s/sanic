#ifndef SANIC_LOG_H
#define SANIC_LOG_H

#include <stdio.h>
#include <time.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_MAGENTA    "\x1b[35m"
#define ANSI_COLOR_GRAY    "\x1b[37m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#ifndef SOURCE_PATH_SIZE
#define FILENAME __FILE__
#else
#define FILENAME (__FILE__ + SOURCE_PATH_SIZE)
#endif

enum sanic_log_level_enum {
    LEVEL_DEBUG,
    LEVEL_TRACE,
    LEVEL_INFO,
    LEVEL_WARN,
    LEVEL_ERROR
};

#ifdef DEFINE_LOG_LEVEL
enum sanic_log_level_enum sanic_log_level = LEVEL_TRACE;
#else
extern enum sanic_log_level_enum sanic_log_level;
#endif

#define sanic_if_log_level(level, action) if(sanic_log_level <= (level)) { action; }
#define sanic_get_time_to_buff \
time_t __now = time(NULL);       \
struct tm __tm_now ;             \
localtime_r(&__now, &__tm_now);    \
char __buff[100];                \
strftime(__buff, sizeof(__buff), "%Y-%m-%d %H:%M:%S", &__tm_now) ;

#define sanic_log_fmt(str, level_str, color, ...) \
ANSI_COLOR_GRAY "%s " color "[" level_str "]\t" ANSI_COLOR_RESET \
ANSI_COLOR_GRAY "%s:%d\t" ANSI_COLOR_RESET        \
str "\n", __buff, FILENAME, __LINE__, __VA_ARGS__

#define sanic_fmt_log_debug(str, ...) sanic_if_log_level(LEVEL_DEBUG, sanic_get_time_to_buff \
printf(sanic_log_fmt(str, "DEBUG", ANSI_COLOR_MAGENTA, __VA_ARGS__)))
#define sanic_log_debug(str) sanic_fmt_log_debug(str, NULL)
#define sanic_fmt_log_debug_req(req, str, ...) sanic_if_log_level(LEVEL_DEBUG, sanic_get_time_to_buff \
char __str_buff[500]; \
sprintf(__str_buff, str, __VA_ARGS__);\
printf(sanic_log_fmt("%-50s req_id=%s fd=%d", "DEBUG", ANSI_COLOR_MAGENTA, __str_buff, (req)->req_id, (req)->conn_fd)))

#define sanic_fmt_log_trace(str, ...) sanic_if_log_level(LEVEL_TRACE, sanic_get_time_to_buff \
printf(sanic_log_fmt(str, "TRACE", ANSI_COLOR_CYAN, __VA_ARGS__)))
#define sanic_log_trace(str) sanic_fmt_log_trace(str, NULL)
#define sanic_fmt_log_trace_req(req, str, ...) sanic_if_log_level(LEVEL_TRACE, sanic_get_time_to_buff \
char __str_buff[500]; \
sprintf(__str_buff, str, __VA_ARGS__);\
printf(sanic_log_fmt("%-50s req_id=%s fd=%d", "TRACE", ANSI_COLOR_CYAN, __str_buff, (req)->req_id, (req)->conn_fd)))
#define sanic_log_trace_req(req, str) sanic_if_log_level(LEVEL_TRACE, sanic_get_time_to_buff \
printf(sanic_log_fmt("%-50s req_id=%s fd=%d", "TRACE", ANSI_COLOR_CYAN, str, (req)->req_id, (req)->conn_fd)))

#define sanic_fmt_log_info(str, ...) sanic_if_log_level(LEVEL_INFO, sanic_get_time_to_buff \
printf(sanic_log_fmt(str, "INFO", ANSI_COLOR_GREEN, __VA_ARGS__)))
#define sanic_log_info(str) sanic_fmt_log_info(str, NULL)
#define sanic_fmt_log_info_req(req, str, ...) sanic_if_log_level(LEVEL_INFO, sanic_get_time_to_buff \
char __str_buff[500]; \
sprintf(__str_buff, str, __VA_ARGS__);\
printf(sanic_log_fmt("%-50s req_id=%s fd=%d", "INFO", ANSI_COLOR_GREEN, __str_buff, (req)->req_id, (req)->conn_fd)))

#define sanic_fmt_log_warn(str, ...) sanic_if_log_level(LEVEL_WARN, sanic_get_time_to_buff \
printf(sanic_log_fmt(str, "WARN", ANSI_COLOR_YELLOW, __VA_ARGS__)))
#define sanic_log_warn(str) sanic_fmt_log_warn(str, NULL)

#define sanic_fmt_log_error(str, ...) sanic_if_log_level(LEVEL_ERROR, sanic_get_time_to_buff \
fprintf(stderr, sanic_log_fmt(str, "ERROR", ANSI_COLOR_RED, __VA_ARGS__)))
#define sanic_log_error(str) sanic_fmt_log_error(str, NULL)

#endif //SANIC_LOG_H
