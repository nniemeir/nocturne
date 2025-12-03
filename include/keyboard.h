/**
 * keyboard.h
 *
 * Keyboard device management and configuration.
 *
 * OVERVIEW:
 * Handles keyboard devices in the compositor. Each physical keybaord gets a
 * tinywl_keyboard struct taht tracks its state and event listeners.
 *
 * When a keyboard is connected, we configure:
 * - Keymap: Layout
 * - Repeat rate: How fast keys repeat when held
 * - Repeat delay: How long to wait before repeating starts
 *
 * KEYMAP:
 * A keymap translates physical key positions to key symbols (keysyms).
 * XKB (X KeyBoard extension) is the standard system for keyboard layouts,
 * originally from X11 but used by Wayland too.
 *
 *
 * KEY EVENTS:
 * Each keyboard can emit several event types:
 * - key: A key was pressed or released
 * - modifiers: Modifier state changed (Shift, Ctrl, Alt, etc.)
 * - destroy: The keyboard was disconnected
 *
 * MULTIPLE KEYBOARDS:
 * The compositor can have multiple keyboards connected simultaneously. All
 * keyboards in the server->keyboards list are active. The seat only has one
 * "active" keyboard at a time, but we can switch which one that is.
 *
 * MODIFIER KEYS:
 * Modifiers are special keys that change the behavior of other keys:
 * - Shift: Capitalizes letters, accesses shifted symbols
 * - Ctrl: Control key for shortcuts
 * - Alt: Alternate key (default modifier key for keybindings in Nocturne)
 * - Logo: Super/Windows/Command key
 * - Caps Lock, Num Lock, etc.
 *
 * Modifier state is tracked separately from regular keys because:
 * - Clients need to know modifier state for key events
 * - Modifiers can be "locked" (like Caps Lock)
 * - Some modifiers have left/right variants
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <wayland-server-core.h>

#include "server.h"

/**
 * struct tinywl_keyboard - Represents a keyboard input device
 * @link: List node for server->keyboards list
 * @server: Back-pointer to the compositor server
 * @wlr_keyboard: The underlying wlroots keyboard object
 * @modifiers: Listeners for modifier key state changes
 * @key: Listener for key press/release events
 * @destroy: Listener for keyboard disconnect events
 *
 * Each physical keyboard gets one of these structs, it tracks:
 * - Which server it belongs to
 * - The wlroots keyboard object
 * - Event listeners for the various keyboard events
 */
struct tinywl_keyboard {
  /* Linked list node. All keyboards are in server->keyboards list */
  struct wl_list link;

  /* Back-pointer to the server that this keyboard belongs to */
  struct tinywl_server *server;
  
  /* The underlying wlroots keyboard object*/
  struct wlr_keyboard *wlr_keyboard;

  /* Event listeners - these call our handler functions when events occur */
  struct wl_listener modifiers; // Called when Shift, Ctrl, Alt, etc. change
  struct wl_listener key; // Called when a key is pressed or release
  struct wl_listener destroy; // Called when keyboard is disconnected
};

/**
 * server_new_keyboard - Handles new keyboard device
 * @server: Server state structure
 * @device: The input device that was connected
 *
 * Called when a keyboard is connected. This function:
 * - Allocates a tinywl_keyboard struct
 * - Creates an XKB keymap with default layout (US)
 * - Configures key repeat (25 repeats/sec, 600ms delay)
 * - Registers event listeners for key, modifiers, destroy
 * - Sets this keyboard as the seat's active keyboard
 * - Adds keyboard to server->keyboards list
 */
void server_new_keyboard(struct tinywl_server *server,
                         struct wlr_input_device *device);

#endif
