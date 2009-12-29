/*
 * timeval.h
 *
 * Copyright (C) 2006, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 */

/* timeval.h: This provides with some timeval functions if not provided
 *            by the system.
 */

#ifndef __TIMEVAL_H
#define __TIMEVAL_H

#ifndef _GNU_SOURCE

#include <sys/time.h>

void
timersub(struct timeval *a, struct timeval *b, struct timeval *res);

int
timerisset(struct timeval *tv);

void
timerclear(struct timeval *tv);

#define timercmp(a, b, CMP) \
	(((a)->tv_sec == (b)->tv_sec) ? \
		((a)->tv_usec CMP (b)->tv_usec) \
		: ((a)->tv_sec CMP (b)->tv_sec))

#endif /* #ifndef _GNU_SOURCE */

#endif
