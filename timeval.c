/*
 * timeval.c
 *
 * Copyright (C) 2006, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 */

/* timeval.c: This provides with some timeval functions if not provided
 *            by the system.
 */

#include "timeval.h"

#ifndef _GNU_SOURCE

void
timersub(struct timeval *a, struct timeval *b, struct timeval *res)
{
	if (a->tv_usec >= b->tv_usec) {
		res->tv_usec = a->tv_usec - b->tv_usec;
		res->tv_sec = a->tv_sec - b->tv_sec;
	} else {
		res->tv_usec = a->tv_usec - b->tv_usec + 1000000;
		res->tv_sec = a->tv_sec - b->tv_sec - 1;
	}
}

int
timerisset(struct timeval *tv)
{
	return ((tv->tv_sec != 0) || (tv->tv_usec != 0));
}

void
timerclear(struct timeval *tv)
{
	tv->tv_sec = 0;
	tv->tv_usec = 0;
}

#endif /* #ifndef _GNU_SOURCE */
