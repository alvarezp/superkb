/*
 * debug.h
 *
 * Copyright (C) 2008, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 * Provides a unified way to print debug messages along the whole code.
 *
 * It does so by providing debug(). It should be used instead of *printf().
 *
 */

#ifndef __DEBUG_H
#define __DEBUG_H

typedef void (debug_t)(const int level, const char *fmt, ...);

void debug(const int level, const char *fmt, ...);

typedef enum {
	KEYBOARD_ACTIONS = 1,
	VARIABLE_CHANGE_VALUES,
	FUNCTION_CALLS,
	VARIABLE_PRINT
} debug_levels_t;

extern int running_debug_level;

#endif
