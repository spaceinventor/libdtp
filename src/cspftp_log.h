#pragma once

#include <stdio.h>
#include <stdbool.h>

extern void dbg_log(const char *__restrict __format, ...);
extern void dbg_warn(const char *__restrict __format, ...);
extern void dbg_enable_colors(bool enable);
