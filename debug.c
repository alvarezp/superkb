/*
 * debug.c
 *
 * Copyright (C) 2008, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 */

#include <stdio.h>
#include <stdarg.h>

#include "debug.h"

int running_debug_level = 0;

/* When you call "debug", you set the level to show the message at.
 * level=1, means an important debug message.
 * level=MAX_INT, means a very deep and obscure message.
 */
void debug(const int level, const char *fmt, ...)
{
	if (level > running_debug_level)
		return;

	va_list args;

	va_start(args, fmt);

	vprintf(fmt, args);

	va_end(args);
}

int debug_level;
