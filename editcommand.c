#include "buffer.h"
#include "editcommand.h"
#include "ui.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

void ec_cursor_forward(EDIT_COMMAND_ARG_LIST) { buffer_cursor_forward(buf); }

void ec_cursor_back(EDIT_COMMAND_ARG_LIST) { buffer_cursor_back(buf); }

void ec_delete_backward(EDIT_COMMAND_ARG_LIST) { buffer_delete_backward(buf); }

void ec_buffer_save(EDIT_COMMAND_ARG_LIST) { buffer_save(buf); }

void ec_editor_quit(EDIT_COMMAND_ARG_LIST) { exit_editor(); }

#pragma GCC diagnostic pop
