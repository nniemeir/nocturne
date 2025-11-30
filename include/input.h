#ifndef INPUT_H
#define INPUT_H

#include "server.h"

void server_new_pointer(struct tinywl_server *server,
                        struct wlr_input_device *device);
void server_new_input(struct wl_listener *listener, void *data);

void seat_request_cursor(struct wl_listener *listener, void *data);

void seat_request_set_selection(struct wl_listener *listener,
                                       void *data);


void reset_cursor_mode(struct tinywl_server *server);
void process_cursor_move(struct tinywl_server *server);
void process_cursor_resize(struct tinywl_server *server);
void process_cursor_motion(struct tinywl_server *server, uint32_t time);
void server_cursor_button(struct wl_listener *listener, void *data);
void server_cursor_motion(struct wl_listener *listener, void *data);
void server_cursor_motion_absolute(struct wl_listener *listener, void *data);
void server_cursor_axis(struct wl_listener *listener, void *data);
void server_cursor_frame(struct wl_listener *listener, void *data);
void begin_interactive(struct tinywl_toplevel *toplevel,
                       enum tinywl_cursor_mode mode, uint32_t edges);

/**
 * struct tinywl_keyboard - Represents a keyboard input device
 *
 * Tracks a single keyboard, its state, and listeners for key presses,
 * modifiers, etc.
 */
struct tinywl_keyboard {
  struct wl_list link;
  struct tinywl_server *server;
  struct wlr_keyboard *wlr_keyboard;

  struct wl_listener modifiers;
  struct wl_listener key;
  struct wl_listener destroy;
};

void server_new_keyboard(struct tinywl_server *server,
                         struct wlr_input_device *device);


#endif
