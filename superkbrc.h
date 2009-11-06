/*
 * conf.h
 *
 * Copyright (C) 2006, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <X11/Xlib.h>
#include "drawkblib.h"

enum action_type {
    AT_COMMAND = 1,
    AT_FUNCTION,
	AT_DOCUMENT
};

/* key_bindings is a dynamic list of keybindings */
struct key_bindings {
    KeyCode keycode;            /* Like in XKeyEvent. */
    unsigned int state;         /* Like in XKeyEvent. */
    unsigned int statemask;
    enum action_type action_type;
    union {
        void (*function)(void *p);
        char *command;
		char *document;
    } action;
    char *icon;
    char *feedback_string;
    /* FIXME: Implement startup notification. */
    /* FIXME: Implement tooltips. */
};

typedef struct {
	struct key_bindings *key_bindings;
	unsigned int key_bindings_n;
	double drawkb_delay;
	char drawkb_font[500];
	char drawkb_imagelib[500];
	KeyCode superkb_super1;
	KeyCode superkb_super2;
	struct {
		int red;
		int green;
		int blue;
	} backcolor;
	struct {
		int red;
		int green;
		int blue;
	} forecolor;
	char document_handler[500];
	int superkb_superkey_replay;
	char feedback_handler[500];
	int superkb_superkey_release_cancels;
	painting_mode_t drawkb_painting_mode;
} config_t;

int config_load(config_t *config, Display *dpy);
config_t * config_new (Display *dpy);

#endif
