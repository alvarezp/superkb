/*
 * debug.h
 *
 * Copyright (C) 2008, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 */

#ifndef __DEBUG_H
#define __DEBUG_H

void debug(int level, char *fmt, ...);

extern int running_debug_level;

#endif
