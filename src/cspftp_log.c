#include <stdarg.h>
#include "cspftp_log.h"

#define YELLOW "\033[0;33m"
#define GREEN "\033[0;32m"
#define RESET "\033[0m"

static const char *yellow = YELLOW;
static const char *green = GREEN;
static const char *reset = RESET;

extern void dbg_log_impl(const char *file, unsigned int line, const char *__restrict __format, ...) {
    fprintf(stdout, "%s[LOG] %s:%u: ", green, file, line);
    va_list start;
    va_start(start, __format);
    vfprintf(stdout, __format, start);
    va_end(start);
    fprintf(stdout, "%s\n", reset); fflush(stdout);
}

extern void dbg_warn_impl(const char *file, unsigned int line, const char *__restrict __format, ...) {
    fprintf(stdout, "%s[WARN] %s:%u: ", yellow, file, line);
    va_list start;
    va_start(start, __format);
    vfprintf(stdout, __format, start);
    va_end(start);
    fprintf(stdout, "%s\n", reset); fflush(stdout);
}

extern void dbg_enable_colors(bool enable) {
    if (enable) {
        yellow = YELLOW;
        green = GREEN;
        reset = RESET;
    } else {
        yellow = "";
        green = "";
        reset = "";
    }
}

#define LOG(x...) do { fprintf(stdout, "%s", green); fprintf(stdout, x); fprintf(stdout, "%s\n", reset); fflush(stdout); } while (0);
#define WARN(x...) do { fprintf(stdout, "%s", yellow); fprintf(stdout, x); fprintf(stdout, "%s\n", reset); fflush(stdout); } while (0);

