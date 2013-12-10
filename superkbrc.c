/*
 * conf.c
 *
 * Copyright (C) 2006, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 * Some code by Nathan "Whatah" Zohar.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <X11/Xlib.h>

#include "superkbrc.h"
#include "globals.h"

#define CONFIG_FILENAME ".superkbrc"

#define CONFIG_DEFAULT_SUPERKB_SUPER1 XKeysymToKeycode(dpy, XStringToKeysym("Super_L"));
#define CONFIG_DEFAULT_SUPERKB_SUPER2 XKeysymToKeycode(dpy, XStringToKeysym("Super_R"));
#define CONFIG_DEFAULT_SQUASHED_STATES 82
#define CONFIG_DEFAULT_DRAWKB_DELAY 0.5
#define CONFIG_DEFAULT_DRAWKB_FONT "Sans Bold"
#define CONFIG_DEFAULT_DRAWKB_IMAGELIB "imlib2"
#define CONFIG_DEFAULT_DRAWKB_DRAWKBLIB "cairo"
#define CONFIG_DEFAULT_DRAWKB_BACKCOLOR_RED 12700
#define CONFIG_DEFAULT_DRAWKB_BACKCOLOR_GREEN 12700
#define CONFIG_DEFAULT_DRAWKB_BACKCOLOR_BLUE 12700
#define CONFIG_DEFAULT_DRAWKB_FORECOLOR_RED 56100
#define CONFIG_DEFAULT_DRAWKB_FORECOLOR_GREEN 56100
#define CONFIG_DEFAULT_DRAWKB_FORECOLOR_BLUE 56100
#define CONFIG_DEFAULT_DRAWKB_USE_GRADIENTS 1
#define CONFIG_DEFAULT_DOCUMENT_HANDLER "gnome-open"
#define CONFIG_DEFAULT_SUPERKB_SUPERKEY_REPLAY 1
#define CONFIG_DEFAULT_FEEDBACK_HANDLER "xmessage -buttons '' -center -timeout 2 Launching "
#define CONFIG_DEFAULT_SUPERKB_SUPERKEY_RELEASE_CANCELS 0
#define CONFIG_DEFAULT_DRAWKB_PAINTING_MODE FLAT_KEY
#define CONFIG_DEFAULT_WELCOME_CMD "xmessage -buttons '' -center -timeout 5 Welcome to Superkb! To start, hold down any of your configured Super keys."

int cver = 0;

config_t * __config;
Display * __dpy;

void
config_add_binding_command(Display *dpy, config_t *this, KeySym keysym, unsigned int state,
			  enum action_type action_type, const char *command,
			  const char *icon, const char *feedback_string)
{
	debug(5, "[bc] Binding command: %d, %s, %s\n", keysym, command, feedback_string);
	list_add_element(this->key_bindings, this->key_bindings_n, struct key_bindings);
	this->key_bindings[this->key_bindings_n - 1].keycode =
		XKeysymToKeycode(dpy, keysym);
	this->key_bindings[this->key_bindings_n - 1].state = state;
	 this->key_bindings[this->key_bindings_n - 1].action_type = action_type;
	this->key_bindings[this->key_bindings_n - 1].action.command = malloc(strlen(command)+1);
	strcpy(this->key_bindings[this->key_bindings_n - 1].action.command, command);

	if (icon != NULL) {
		this->key_bindings[this->key_bindings_n - 1].icon = malloc(strlen(icon)+1);
		strcpy(this->key_bindings[this->key_bindings_n - 1].icon, icon);
	} else {
		this->key_bindings[this->key_bindings_n - 1].icon = NULL;
	}

	if (feedback_string != NULL) {
		this->key_bindings[this->key_bindings_n - 1].feedback_string = malloc(strlen(feedback_string)+1);
		strcpy(this->key_bindings[this->key_bindings_n - 1].feedback_string, feedback_string);
	} else {
		this->key_bindings[this->key_bindings_n - 1].feedback_string = NULL;
	}
}

void
config_add_binding_document(Display *dpy, config_t *this, KeySym keysym, unsigned int state,
			  enum action_type action_type, const char *document,
			  const char *icon, const char *feedback_string)
{
	list_add_element(this->key_bindings, this->key_bindings_n, struct key_bindings);
	this->key_bindings[this->key_bindings_n - 1].keycode =
		XKeysymToKeycode(dpy, keysym);
	this->key_bindings[this->key_bindings_n - 1].state = state;
	 this->key_bindings[this->key_bindings_n - 1].action_type = action_type;
	this->key_bindings[this->key_bindings_n - 1].action.document = malloc(strlen(document)+1);
	strcpy(this->key_bindings[this->key_bindings_n - 1].action.document, document);

	if (icon != NULL) {
		this->key_bindings[this->key_bindings_n - 1].icon = malloc(strlen(icon)+1);
		strcpy(this->key_bindings[this->key_bindings_n - 1].icon, icon);
	} else {
		this->key_bindings[this->key_bindings_n - 1].icon = NULL;
	}

	if (feedback_string != NULL) {
		this->key_bindings[this->key_bindings_n - 1].feedback_string = malloc(strlen(feedback_string)+1);
		strcpy(this->key_bindings[this->key_bindings_n - 1].feedback_string, feedback_string);
	} else {
		this->key_bindings[this->key_bindings_n - 1].feedback_string = NULL;
	}
}

void
config_add_binding(Display *dpy, config_t *this, enum action_type binding_type, KeySym keysym, unsigned int state,
			  enum action_type action_type, const char *content,
			  const char *icon, const char *feedback_string, int autoquote)
{

	char *feedback_string_quoted = (char *)feedback_string;

	int already_quoted = 0;
	if (feedback_string != NULL) {
		if (feedback_string[0] == '\'' && feedback_string[strlen(feedback_string) - 1] == '\'') {
			debug(4, "[aq] Already quoted: %s\n", feedback_string);
			already_quoted = 1;
		} else {
			debug(4, "[aq] Will try to quote: %s\n", feedback_string);
		}
	}

	if (autoquote && !already_quoted && feedback_string != NULL) {
		feedback_string_quoted = malloc(strlen(feedback_string) + 3);
		if (feedback_string_quoted) {
			strcpy(feedback_string_quoted, "'");
			strcat(feedback_string_quoted, feedback_string);
			strcat(feedback_string_quoted, "'");
			debug(4, "[aq] New quoted string: %s\n", feedback_string_quoted);
		} else {
			feedback_string_quoted = (char *)feedback_string;
			debug(4, "[aq] Could not get memory for new string. Falling back: %s\n", feedback_string_quoted);
		}
	}

	switch (binding_type) {
	case AT_DOCUMENT:
		debug(4, "[aq] AT_DOCUMENT: %s, %s\n", feedback_string_quoted, icon);
		config_add_binding_document(dpy, this, keysym, state, AT_DOCUMENT, content, icon, feedback_string_quoted);
		break;
	case AT_COMMAND:
		debug(4, "[aq] AT_COMMAND: %s, %s\n", feedback_string_quoted, icon);
		config_add_binding_command(dpy, this, keysym, state, AT_COMMAND, content, icon, feedback_string_quoted);
		break;
	default:
		break;
	}
}

/* lets us know if the line is blank or not */
int empty(char *string, int size) {
	int i;
	for (i = 0; i < size; i++) {
		if (!isspace(string[i])) {
			return 0;
		}
	}
	return 1;
}

/** Return address of next word in buf and update *wordLength
 *  to the length of that word.  Return NULL if there is no such
 *  word. 
 * 
 * A word is defined as all the chars from buf until one in delim and
 *  and until the next item in delim. (it strips whitespace out of the word)
 *
 *  If the first character is a double quote, then it will search for the 
 *  next double quote as a delimiter.
**/
const char *next_word(const char *buf, int *wordLength, const char* delim) {
	char c = *buf;
	int i = 0; /* Word length counter */
	int j = 0; /* Leading whitespace counter */
	
	while (strchr(delim, c) != NULL && c != '\0') {
		c = buf[j++];
	}

	if (j > 0) j--;

	if (buf[j] == '\0') return NULL;

	if (buf[j] == '"') {
		c = buf[++j];
		i = j;
		while (c != '"' && c != '\0') {
			c = buf[++i];
		}
		i++;
	} else {
		i = j;
		while (strchr(delim, c) == NULL) {
			c = buf[i++];
		}
	}
	
	*wordLength = --i - j;
	return buf + j;
}


/** Read next line (terminated by `\n` or EOF) from in into dynamically
 *  allocated buffer buf having size *bufSize.  The line is terminated
 *  by a NUL ('\0') character.  The terminating newline is not
 *  included.  If buf is not large enough to contain
 *  the line then it is reallocated and the new size stored in *bufSize.
 *  The return value is a pointer to the start of the buffer containing
 *  the line.  Sets *isEof to non-zero on EOF.  Aborts with an
 *  error message on error.
**/
char *get_line(FILE *in, char *buf, int *bufSize, int *isEof) {
	int c;
	int i = 0;

	if (*bufSize == 0) *bufSize = 1;
	buf = realloc(buf, *bufSize * sizeof(char));

	while((c = fgetc(in)) != EOF && (c != '\n')) {

		if (i >= (*bufSize) - 1) {
			buf = realloc(buf, sizeof(char) * *bufSize * 2);
			*bufSize *= 2;
		}
		buf[i++] = c;
	}
	
	if (c == EOF) {
		*isEof = 1;
	}
	
	if (ferror(in)) {
		perror("superkb: ");
		exit(EXIT_FAILURE);
	}

	buf[i] = 0;
	*bufSize = i;

	return buf;
}

/* Reads line and updates **c to reflect the configuration of that line */
void handle_line(char *line, int linesize) {

	static int autoquote = 1;

	char *comment;
	/* We zero out anything past a '#', including the '#', for commenting */
	if ((comment = strchr(line, '#')) != NULL) {
		*comment = 0;
		linesize = (comment - line);
	}
	
	/* Sanity Checks */
	if (linesize == 0) return;
	if (empty(line, linesize)) return;
	
	/* Tokenize the line by whitespace, filling token_array with tok_index
	 * number of items. */

	int wordlength;
	char **token_array = malloc(sizeof(*token_array));
	char *token;
	char *token_item;
	int tok_size;
	int tok_index;
	
	int q;

	wordlength = -1;
	token = line;
	tok_index = 0;
	tok_size = 1;

	
	while ((token = (char *) next_word(token + wordlength + 1, &wordlength, " \v\t\r")) != NULL) {
		if (tok_size <= tok_index) {
			void *new_token_array = realloc(token_array, tok_size * 2 * sizeof(*token_array));
			if (new_token_array == NULL) {
				fprintf(stderr, "superkbrc.c: error allocating memory for tokenizer. Aborting.\n");
				abort();
			}
			token_array = new_token_array;
			tok_size *= 2;
		}
		/* Need to end each token with a null, so add 1 to the wordlength and NULL it */
		token_item = malloc(sizeof(*token_item) * wordlength + 1);
		/* copy the token into our newly allocated space */
		memcpy(token_item, token, wordlength);
		token_item[wordlength] = 0;
		/* pop it into the array */
		token_array[tok_index++] = token_item;

		if (token + wordlength >= line + linesize) {
			break;
		}
	}

	/* Finished tokenizing */

	/* (Octavio) Interpretation */

	if (!strcmp(token_array[0], "CVER"))
	{
		int input;

		/* FIXME: This will accept strings like '2a'. Though it will fallback
		 * correctly to '2', it might not be what the user wants. It should
		 * spit back a warning or an error.
		 */
		input = atoi(token_array[1]);

		if (input > 0) {
			fprintf(stderr, "Ignoring bad CVER: %d\n", atoi(token_array[1]));
		}
	} else if (cver == 0) {

		if (!strcmp(token_array[0], "DELAY") && tok_index == 2) {
			__config->drawkb_delay = atof(token_array[1]);
			return;
		}

		if (!strcmp(token_array[0], "FONT") && tok_index == 2) {
			strncpy(__config->drawkb_font, token_array[1], 500);
			return;
		}

		if (!strcmp(token_array[0], "IMAGELIB") && tok_index == 2) {
			strncpy(__config->drawkb_imagelib, token_array[1], 500);
			return;
		}

		if (!strcmp(token_array[0], "DRAWKBLIB") && tok_index == 2) {
			strncpy(__config->drawkb_drawkblib, token_array[1], 500);
			return;
		}

		if (!strcmp(token_array[0], "DOCUMENT_HANDLER") && tok_index == 2) {
			strncpy(__config->document_handler, token_array[1], 500);
			return;
		}

		if (!strcmp(token_array[0], "SUPERKEY1_STRING") && tok_index == 2) {
			__config->superkb_super1 = XKeysymToKeycode(__dpy, XStringToKeysym(token_array[1]));
			return;
		}

		if (!strcmp(token_array[0], "SUPERKEY2_STRING") && tok_index == 2) {
			__config->superkb_super2 = XKeysymToKeycode(__dpy, XStringToKeysym(token_array[1]));
			return;
		}

		if (!strcmp(token_array[0], "SUPERKEY1_CODE") && tok_index == 2) {
			__config->superkb_super1 = atoi(token_array[1]);
			return;
		}

		if (!strcmp(token_array[0], "SUPERKEY2_CODE") && tok_index == 2) {
			__config->superkb_super2 = atoi(token_array[1]);
			return;
		}

		if (!strcmp(token_array[0], "SQUASHED_STATES") && tok_index == 2) {
			__config->squashed_states = atoi(token_array[1]);
			return;
		}

		if (!strcmp(token_array[0], "BACKGROUND") && tok_index == 4) {
			
			__config->backcolor.red = atoi(token_array[1]);
			__config->backcolor.green = atoi(token_array[2]);
			__config->backcolor.blue = atoi(token_array[3]);
			if (__config->backcolor.red <= 255 &&
				__config->backcolor.green <= 255 &&
				__config->backcolor.blue <= 255) {
				__config->backcolor.red *= 256;
				__config->backcolor.green *= 256;
				__config->backcolor.blue *= 256;
			}
			return;
		}

		if (!strcmp(token_array[0], "FOREGROUND") && tok_index == 4) {
			__config->forecolor.red = atoi(token_array[1]);
			__config->forecolor.green = atoi(token_array[2]);
			__config->forecolor.blue = atoi(token_array[3]);
			if (__config->forecolor.red <= 255 &&
				__config->forecolor.green <= 255 &&
				__config->forecolor.blue <= 255) {
				__config->forecolor.red *= 256;
				__config->forecolor.green *= 256;
				__config->forecolor.blue *= 256;
			}
			return;
		}

		if (!strcmp(token_array[0], "SUPERKEY_REPLAY") && tok_index == 2) {
			if (!strcmp(token_array[1], "1"))
				__config->superkb_superkey_replay = 1;
			else if (!strcmp(token_array[1], "0"))
				__config->superkb_superkey_replay = 0;
			else fprintf(stderr, "superkb: SUPERKEY_REPLAY value must be 1 or 0.\n");
			return;
		}

		if (!strcmp(token_array[0], "FEEDBACK_STRINGS_AUTOQUOTE") && tok_index == 2) {
			if (!strcmp(token_array[1], "1"))
				autoquote = 1;
			else if (!strcmp(token_array[1], "0"))
				autoquote = 0;
			else fprintf(stderr, "superkb: FEEDBACK_STRINGS_AUTOQUOTE must be 1 or 0.\n");
			return;
		}

		if (!strcmp(token_array[0], "SUPERKEY_RELEASE_CANCELS") && tok_index == 2) {
			if (!strcmp(token_array[1], "1"))
				__config->superkb_superkey_release_cancels = 1;
			else if (!strcmp(token_array[1], "0"))
				__config->superkb_superkey_release_cancels = 0;
			else fprintf(stderr, "superkb: SUPERKEY_RELEASE_CANCELS value must be 1 or 0.\n");
			return;
		}

		if (!strcmp(token_array[0], "DRAWKB_PAINTING_MODE") && tok_index == 2) {
			if (!strcmp(token_array[1], "FULL_SHAPE"))
				__config->drawkb_painting_mode = FULL_SHAPE;
			else if (!strcmp(token_array[1], "BASE_OUTLINE_ONLY"))
				__config->drawkb_painting_mode = BASE_OUTLINE_ONLY;
			else if (!strcmp(token_array[1], "FLAT_KEY"))
				__config->drawkb_painting_mode = FLAT_KEY;
			else fprintf(stderr, "superkb: DRAWKB_PAINTING_MODE value must be one of: FULL_SHAPE, BASE_OUTLINE_ONLY, FLAT_KEY.\n");
			return;
		}

		if (!strcmp(token_array[0], "FEEDBACK_HANDLER") && tok_index == 2) {
			strncpy(__config->feedback_handler, token_array[1], 500);
			return;
		}

		if (!strcmp(token_array[0], "WELCOME_CMD") && tok_index == 2) {
			strncpy(__config->welcome_cmd, token_array[1], 500);
			return;
		}

		if (!strcmp(token_array[0], "USE_GRADIENTS") && tok_index == 2) {
			if (!strcmp(token_array[1], "1"))
				__config->use_gradients = 1;
			else if (!strcmp(token_array[1], "0"))
				__config->use_gradients = 0;
			else fprintf(stderr, "superkb: USE_GRADIENTS value must be 1 or 0.\n");
			return;
		}


		/* FIXME: There might not exist token_array[1]. */
		if (!strcmp(token_array[0], "KEY")
			&& !strcmp(token_array[1], "COMMAND")
			&& (tok_index == 6 || tok_index == 7))
		{
			/* FIXME: Key validation missing. */

			/* Check if file exists */

			char *validated_param;
			FILE *filecheck;
			filecheck = fopen (token_array[5], "r");
			if (filecheck == NULL)   {
				validated_param = NULL;
				fprintf(stderr, "superkb: Specified icon file does not exist: %s\n",
					token_array[5]);
				
			} else {
				fclose (filecheck);
				validated_param = (char *) token_array[5];
			}

			config_add_binding(__dpy, __config, AT_COMMAND,
				XStringToKeysym(token_array[2]), atoi(token_array[3]),
				AT_COMMAND, token_array[4], validated_param, tok_index == 7 ? token_array[6] : NULL, autoquote);
		} else if (!strcmp(token_array[0], "KEY")
			&& !strcmp(token_array[1], "DOCUMENT")
			&& (tok_index == 6 || tok_index == 7))
		{
			/* FIXME: Key validation missing. */

			/* Check if file exists */

			char *validated_param;
			FILE *filecheck;
			filecheck = fopen (token_array[5], "r");
			if (filecheck == NULL)   {
				validated_param = NULL;
				fprintf(stderr, "superkb: Specified icon file does not exist: %s\n",
					token_array[5]);
				
			} else {
				fclose (filecheck);
				validated_param = (char *) token_array[5];
			}

			config_add_binding(__dpy, __config, AT_DOCUMENT,
				XStringToKeysym(token_array[2]), atoi(token_array[3]),
				AT_DOCUMENT, token_array[4], validated_param, tok_index == 7 ? token_array[6] : NULL, autoquote);
		} else {
			fprintf(stderr, "Ignoring invalid config line: '[%s]", token_array[0]);
			for (q = 1; q < tok_index; q++) {
				fprintf(stderr, " [%s]", token_array[q]);
			}
			fprintf(stderr, "'\n");
		}
		
	}

	/* Free our allocated memory */
	
	/* iterate through the token_array freeing everything */
	for (q = 0; q < tok_index; q++) {
		free(token_array[q]);
	}
	
	free(token_array);
	
	return;
}

int parse_config(FILE *file) {
	char *buf = malloc(sizeof(*buf));
	if (buf == NULL) {
		perror("superkb: parse_config(): buf = malloc() failed");
		abort();
	}

	int *bufSize = malloc(sizeof(*bufSize));
	if (bufSize == NULL) {
		perror("superkb: parse_config(): bufSize = malloc() failed");
		abort();
	}

	int *eof = malloc(sizeof(*eof));
	if (eof == NULL) {
		perror("superkb: parse_config(): eof = malloc() failed");
		abort();
	}
 
	*bufSize = 1;
	*eof = 0;

	while (*eof == 0) {
		buf = get_line(file, buf, bufSize, eof);
		handle_line(buf, *bufSize);
		free(buf);
		buf=NULL;
	}

	free(buf);
	free(bufSize);
	free(eof);
	return EXIT_SUCCESS;
}

config_t * config_new (Display *dpy)
{

	config_t *this;

	this = malloc(sizeof(config_t));

	if (this == NULL) {
		perror("superkb: config_new(): malloc() failed");
		return NULL;
	}

	/* Initialize everything */
	this->key_bindings = NULL;
	this->key_bindings_n = 0;
	this->drawkb_delay = CONFIG_DEFAULT_DRAWKB_DELAY;
	strcpy(this->drawkb_font, CONFIG_DEFAULT_DRAWKB_FONT);
	strcpy(this->drawkb_imagelib, CONFIG_DEFAULT_DRAWKB_IMAGELIB);
	strcpy(this->drawkb_drawkblib, CONFIG_DEFAULT_DRAWKB_DRAWKBLIB);
	this->superkb_super1 = CONFIG_DEFAULT_SUPERKB_SUPER1;
	this->superkb_super2 = CONFIG_DEFAULT_SUPERKB_SUPER2;
	this->backcolor.red = CONFIG_DEFAULT_DRAWKB_BACKCOLOR_RED;
	this->backcolor.green = CONFIG_DEFAULT_DRAWKB_BACKCOLOR_GREEN;
	this->backcolor.blue = CONFIG_DEFAULT_DRAWKB_BACKCOLOR_BLUE;
	this->forecolor.red = CONFIG_DEFAULT_DRAWKB_FORECOLOR_RED;
	this->forecolor.green = CONFIG_DEFAULT_DRAWKB_FORECOLOR_GREEN;
	this->forecolor.blue = CONFIG_DEFAULT_DRAWKB_FORECOLOR_BLUE;
	this->use_gradients = CONFIG_DEFAULT_DRAWKB_USE_GRADIENTS;
	strcpy(this->document_handler, CONFIG_DEFAULT_DOCUMENT_HANDLER);
	this->superkb_superkey_replay = CONFIG_DEFAULT_SUPERKB_SUPERKEY_REPLAY;
	strcpy(this->feedback_handler, CONFIG_DEFAULT_FEEDBACK_HANDLER);
	this->superkb_superkey_release_cancels = CONFIG_DEFAULT_SUPERKB_SUPERKEY_RELEASE_CANCELS;
	this->drawkb_painting_mode = CONFIG_DEFAULT_DRAWKB_PAINTING_MODE;
	this->squashed_states = CONFIG_DEFAULT_SQUASHED_STATES;
	strcpy(this->welcome_cmd, CONFIG_DEFAULT_WELCOME_CMD);

	return this;

}

void config_destroy (config_t *this)
{
	free(this);
}

int config_load(config_t *this, Display *dpy)
{
	__dpy = dpy;
	__config = this;
	int system_config_read = 0;
	int user_config_read = 0;

	/* Override current configuration with system-wide settings. */
	FILE *fd;

	fd = fopen("/etc/superkbrc", "r");
	if (fd) {
		system_config_read = 0;
		if (parse_config(fd) == EXIT_SUCCESS) {
			system_config_read = 1;
		} else {
			perror("config_load(): parse_config(system) failed");
		}
		fclose(fd);
	}

	/* Read configuration from user file. */
	char *home = getenv("HOME");
	char *file;

	if (home) {
		file = malloc(strlen(home) + strlen("/" CONFIG_FILENAME)+1);
		if (!file) {
			fprintf(stderr, "superkb: Not enough memory to open a file. Very "
				"weird. Quitting.\n");
			return EXIT_FAILURE;
		}
		strcpy(file, home);
		strcat(file, "/" CONFIG_FILENAME);
	} else {
		file = CONFIG_FILENAME;
	}

	fd = fopen(file, "r");
	if (fd) {
		user_config_read = 0;
		if (parse_config(fd) == EXIT_SUCCESS) {
			user_config_read = 1;
		} else {
			perror("config_load(): parse_config(user) failed");
		}
		fclose(fd);
	}

	if (!user_config_read && !system_config_read) {
		fprintf(stderr, "superkb: Couldn't read any configuration file. Quitting.\n");
		fprintf(stderr, "  System-wide configuration file should be at /etc/superkbrc.\n");
		fprintf(stderr, "  User local configuration file should be at $HOME/.superkbrc.\n");
	}

	return EXIT_SUCCESS;
}

