#ifndef KED_INTERNAL_H
#define KED_INTERNAL_H

#include "ked.h"

// extension.c

/* Initializes variables required for use extension feature. */
void extension_set_up(void);

/* Executes tear-down routine for each library if it exists then unload the
 * library. */
void extension_tear_down(void);


// keybind.c

/* Initializes local variables that needed for key bind handling, and global key
 * bind. */
void keybind_set_up(void);
/* Frees allocated for key bind handling. */
void keybind_tear_down(void);

/* Handles key input or buffers the key for next input. */
void handle_key(int c);

/* Clear queued key sequence and insert given Rune to buffer. */
void handle_rune(Rune r);


// ui.c

/* Initializes variables neeed to draw the UI. */
void ui_set_up(void);

/* Frees all buffer. */
void ui_tear_down(void);

/* Main routine of ked. Wait for input and refresh the screen. */
void editor_main_loop(void);

/* Creates buffer for header line and message line. */
void init_system_buffers(void);

/* Forcused buffer. */
extern Buffer *current_buffer;
/* All entries of displayed buffer. */
extern Buffer **displayed_buffers;


// terminal.c

extern size_t term_width;
extern size_t term_height;

/* Initializes terminal for editor functionally. */
void term_set_up(void);
/* Restores original terminal settings. */
void term_tear_down(void);

#endif
