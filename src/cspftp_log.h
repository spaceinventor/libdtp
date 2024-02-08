#pragma once

#include <stdio.h>
#include <stdbool.h>

#define dbg_log(x...) dbg_log_impl(__FILE__, __LINE__, x)
#define dbg_warn(x...) dbg_warn_impl(__FILE__, __LINE__, x)

extern void dbg_log_impl(const char *file, unsigned int line, const char *__restrict __format, ...);
extern void dbg_warn_impl(const char *file, unsigned int line, const char *__restrict __format, ...);
extern void dbg_enable_colors(bool enable);
