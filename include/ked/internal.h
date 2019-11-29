#ifndef KED_INTERNAL_H
#define KED_INTERNAL_H

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


// ui.c

/* Initializes variables neeed to draw the UI. */
void ui_set_up(void);
/* Frees all buffer. */
void ui_tear_down(void);


// terminal.c

/* Initializes terminal for editor functionally. */
void term_set_up(void);
/* Restores original terminal settings. */
void term_tear_down(void);

#endif
