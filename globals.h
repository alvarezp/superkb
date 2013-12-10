/*
 * globals.h
 *
 * Copyright (C) 2006, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 */

#ifndef __GLOBALS_H
#define __GLOBALS_H

/* Wrappers for easy dynamic array element adding and removing. */
#define list_add_element(x, xn, y) do {void * t; t = (y *)realloc(x, (++(xn))*sizeof(y)); if (t == NULL) { free(x); x = NULL; } else x = t; } while (0)
#define list_rmv_element(x, xn, y) do {void * t; t = (y *)realloc(x, (--(xn))*sizeof(y)); if (t == NULL) { free(x); x = NULL; } else x = t; } while (0)

#endif
