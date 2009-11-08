/*
 * debug.h
 *
 * Copyright (C) 2008, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 */

#ifndef __DEBUG_H
#define __DEBUG_H

typedef void (debug_t)(int level, char *fmt, ...);

void debug(int level, char *fmt, ...);

typedef enum {
	KEYBOARD_ACTIONS = 1,
	VARIABLE_CHANGE_VALUES,
	FUNCTION_CALLS,
	VARIABLE_PRINT
} debug_levels_t;

extern int running_debug_level;

#endif
