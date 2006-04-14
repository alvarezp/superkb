    /* License: GPL v2. */
/* To be compiled with:
 *    gcc -o superkb proto.c -ansi -lX11 -L/usr/X11/lib
 */

/* Thanks to Natan "Whatah" Zohar for helping with tokenizer. */

#include <X11/Xlib.h>

#include <X11/XKBlib.h>
#include <X11/extensions/XKBgeom.h>

#include <gdk-pixbuf-xlib/gdk-pixbuf-xlib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>

#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <signal.h>

#include "superkb.h"
#include "drawkb.h"

struct sigaction action;

Window kbwin;
Window confwin;

Window prev_kbwin_focus;
int prev_kbwin_revert;

XkbDescPtr kbdesc;
XkbGeometryPtr kb_geom;

double scale;
GdkPixbuf *kb;

GC gc;

Display *dpy;

int cver = 0;

/* Wrappers for easy dynamic array element adding and removing. */
#define list_add_element(x, xn, y) {x = (y *)realloc(x, (++(xn))*sizeof(y));}
#define list_rmv_element(x, xn, y) {x = (y *)realloc(x, (--(xn))*sizeof(y));}

enum action_type {
    AT_COMMAND = 1,
    AT_FUNCTION
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
    } action;
    char *icon;
    /* FIXME: Implement startup notification. */
    /* FIXME: Implement tooltips. */
} *key_bindings = NULL;

unsigned int key_bindings_n = 0;

int fatal_error(const char * format, ...) {
    va_list args;
    va_start (args, format);
    fprintf(stderr, format, args);
    abort();
    return 0;
}

void IconQuery(KeySym keysym, unsigned int state, char buf[], int buf_n)
{
    int i;
    for (i = 0; i < key_bindings_n; i++)
    {
        if (keysym == XKeycodeToKeysym(dpy, key_bindings[i].keycode, 0)) {
            strncpy(buf, key_bindings[i].icon, buf_n);
        }
    }
}

void kbwin_event(Display * dpy, XEvent ev)
{

    if (ev.type == Expose) {
        KbDrawKeyboard(dpy, kbwin, gc, 0, scale, 0, 0, kbdesc, IconQuery);
        XFlush(dpy);
    } else if (ev.type == VisibilityNotify &&
               ev.xvisibility.state != VisibilityUnobscured) {
        XRaiseWindow(dpy, kbwin);
    }

}

void kbwin_map(Display * dpy)
{
    /* XGetInputFocus(dpy, &prev_kbwin_focus, &prev_kbwin_revert); */
    XMapWindow(dpy, kbwin);
}

void kbwin_unmap(Display * dpy)
{
    XUnmapWindow(dpy, kbwin);
    /* XSetInputFocus(dpy, prev_kbwin_focus, prev_kbwin_revert, CurrentTime); */
}

void kbwin_init(Display * dpy)
{

    gdk_pixbuf_xlib_init(dpy, 0);

/*  kb = gdk_pixbuf_new_from_file ("superkb.png", NULL); */

/*  assert (kb); */

    XkbQueryExtension(dpy, NULL, NULL, NULL, NULL, NULL);

    kbdesc = XkbGetKeyboard(dpy, XkbAllComponentsMask, XkbUseCoreKbd);

    int status;
    status = XkbGetGeometry(dpy, kbdesc);

    kb_geom = kbdesc->geom;

    /* unsigned long black = BlackPixel(dpy, DefaultScreen(dpy)); */
    /* unsigned long white = WhitePixel(dpy, DefaultScreen(dpy)); */

    int winh = DisplayWidth(dpy, 0);
    int winv = DisplayHeight(dpy, 0);

    double scalew = (float) winh / kb_geom->width_mm;
    double scaleh = (float) winv / kb_geom->height_mm;

    if (scalew < scaleh) {
        scale = scalew;
        winv = kb_geom->height_mm * scale;
    } else {
        scale = scaleh;
        winh = kb_geom->width_mm * scale;
    }

    kbwin = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy),
                            (DisplayWidth(dpy, 0) - winh) / 2,
                            (DisplayHeight(dpy, 0) - winv) / 2, winh, winv,
                            0, 0, ((200 << 16) + (200 << 8) + (220)));

    gc = XCreateGC(dpy, kbwin, 0, NULL);

    XSetWindowAttributes attr;
    attr.override_redirect = True;

    XChangeWindowAttributes(dpy, kbwin, CWOverrideRedirect, &attr);

    XSetTransientForHint(dpy, kbwin, DefaultRootWindow(dpy));

    XSelectInput(dpy, kbwin, ExposureMask | VisibilityChangeMask);

    XFlush(dpy);

}

void __Superkb_Action(KeyCode keycode, unsigned int state)
{
    int i;
    for (i = 0; i < key_bindings_n; i++) {
        if (key_bindings[i].keycode == keycode &&
          key_bindings[i].state ==
          (state & key_bindings[i].state)) {
            switch (key_bindings[i].action_type) {
            case AT_COMMAND:
                if (fork() == 0) {
                    system(key_bindings[i].action.command);
                    exit(EXIT_SUCCESS);
                }
                break;
            case AT_FUNCTION:
                if (fork() == 0) {
                    /* FIXME: Value should not be NULL. */
                    key_bindings[i].action.function(NULL);
                    exit(EXIT_SUCCESS);
                }

            }
        }
    }
}

void
add_binding(KeySym keysym, unsigned int state,
              enum action_type action_type, const char *command,
              const char *icon)
{
    list_add_element(key_bindings, key_bindings_n, struct key_bindings);
    key_bindings[key_bindings_n - 1].keycode =
        XKeysymToKeycode(dpy, keysym);
    key_bindings[key_bindings_n - 1].state = state;
     key_bindings[key_bindings_n - 1].action_type = action_type;
    key_bindings[key_bindings_n - 1].action.command = malloc(strlen(command)+1);
    key_bindings[key_bindings_n - 1].icon = malloc(strlen(icon)+1);
    strcpy(key_bindings[key_bindings_n - 1].action.command, command);
    strcpy(key_bindings[key_bindings_n - 1].icon, icon);
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
            c = buf[i++];
        }
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
    char c;
    int i = 0;

    if (*bufSize == 0) *bufSize = 1;
    buf = realloc(buf, *bufSize * sizeof(*buf));

    while((c = fgetc(in)) != EOF && (c != '\n')) {

        ferror(in) && fatal_error("Error reading from file\n");

        if (i == *bufSize) {
            buf = realloc(buf, sizeof(char) * *bufSize * 2);
            *bufSize *= 2;
        }
        buf[i++] = c;
    }
    
    if (c == EOF) {
        *isEof = 1;
    }
    
    buf[i] = 0;
    *bufSize = i;

    return buf;
}

/* Reads line and updates **c to reflect the configuration of that line */
void handle_line(char *line, int linesize) {
    char *comment;
    /* We zero out anything past a '#', including the '#', for commenting */
    if ((comment = strchr(line, '#')) != NULL) {
        *comment = 0;
        linesize = (comment - line);
    }
    
    /* printf("line:%s characters:%i\n", line, linesize); */
    
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
            token_array = realloc(token_array, tok_size * 2 * sizeof(*token_array));
            tok_size *= 2;
        }
        /* Need to end each token with a null, so add 1 to the wordlength and NULL it */
        token_item = malloc(sizeof(*token_item) * wordlength + 1);
        /* copy the token into our newly allocated space */
        memcpy(token_item, token, wordlength);
        token_item[wordlength] = 0;
        /* pop it into the array */
        token_array[tok_index++] = token_item;
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
            drawkb_delay = atof(token_array[1]);
            return;
        }

        /* FIXME: There might not exist token_array[1]. */
        if (!strcmp(token_array[0], "KEY") && !strcmp(token_array[1], "COMMAND"))
        {
            /* FIXME: Do corresponding validation. */
            /* We have to choose whether to validate here or upon display and
             * execution. I guess the later would make the program more robust.
             */
            add_binding(XStringToKeysym(token_array[2]), atoi(token_array[3]), AT_COMMAND, token_array[4], token_array[5]);
        } else {
            fprintf(stderr, "Ignoring invalid config line: '%s", token_array[0]);
            for (q = 1; q < tok_index; q++) {
                printf(" %s", token_array[q]);
            }
            printf("'\n");
        }
        
    }

    /* Free our allocated memory */
    
    /* iterate through the token_array freeing everything */
    for (q = 0; q < tok_index; q++) {
        free(token_array[q]);
    }
    
    free(token_array);
    free(token);
    
    return;
}

void read_config(FILE *file) {
    char *buf = malloc(sizeof(*buf));
    int *bufSize = malloc(sizeof(*bufSize));
    int *eof = malloc(sizeof(*eof));
 
    *bufSize = 1;
    *eof = 0;

    while (*eof == 0) {
        buf = get_line(file, buf, bufSize, eof);
        handle_line(buf, *bufSize);
    }

    return;
}

void sighandler(int sig)
{
    switch (sig) {
    case SIGUSR1:
        break;
    }
}

int main()
{

    printf("\nsuperkb: Welcome. This program is under development.\n"
      "\n"
      "It's strongly recommended to set the following under xorg.conf:\n"
      "  Option \"AllowDeactivateGrabs\" \"On\"\n"
      "  Option \"AllowClosedownGrabs\" \"On\"\n"
      "\n"
      "With these, if case the program fails while drawing the keyboard, you\n"
      "will be able to kill it by pressing Ctrl-Alt-*, and restore Autorepeat\n"
      "with 'xset r on'.\n\n");

    g_type_init();

    /* SIGUSR1: Exit. */
    action.sa_handler = sighandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGUSR1, &action, NULL);

    /* 2. Connect to display. */
    dpy = XOpenDisplay(NULL);
    (!dpy) && fatal_error("superkb: Couldn't open display.\n");

    /* Read the mock config file */
    FILE *fd;

    char *home = getenv("HOME");
    char *file;

    if (home) {
        file = malloc(strlen(getenv("HOME")) + strlen("/.superkbrc") + 1);
        strcpy(file, home);
        !file && fatal_error("superkb: Not enough memory\n");
        strcat(file, "/.superkbrc");
    } else {
        file = ".superkbrc";
    }

    fd = fopen(file, "r");
    (!fd) && fatal_error("superkb: Couldn't open cofig file: %s\n", file);

    read_config(fd);
    fclose(fd);

    superkb_load(dpy, kbwin_init, kbwin_map, kbwin_unmap, kbwin_event,
                 "en", XStringToKeysym("Super_R"),
                 XStringToKeysym("Super_L"), __Superkb_Action);

    superkb_start();

    return EXIT_SUCCESS;
}
