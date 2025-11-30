#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <wayland-server-core.h>

#include "server.h"

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
