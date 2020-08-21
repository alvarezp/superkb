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
#define list_add_element(x, xn, y) {x = (y *)realloc(x, (++(xn))*sizeof(y));}
#define list_rmv_element(x, xn, y) {x = (y *)realloc(x, (--(xn))*sizeof(y));}

#endif
