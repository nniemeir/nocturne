/**
 * config.h
 *
 * Keybinding configuration interface for the compositor.
 *
 * OVERVIEW:
 * Defines the keybinding system for Nocturne. Keybindings are divded into two
 * categories:
 * - Compositor bindings: Actions that control the compositor itself
 * - User bindings: Launching external applications
 *
 * MODIFIER KEY:
 * The MODKEY macro defines which modifier key must be held for keybindings to
 * trigger. Common options:
 * - WLR_MODIFIER_ALT: Alt key (default)
 * - WLR_MODIFIER_LOGO: Super/Windows key
 * - WLR_MODIFIER_CTRL: Control key
 * - WLR_MODIFIER_SHIFT: Shift key
 *
 * KEYSYMS:
 * Keys are identified using xkb_keysym_t values from xkbcommon. These are
 * cross-platform symbolic represenatations of keys.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <xkbcommon/xkbcommon.h>

#include "server.h"

/* Number of compositor-level keybindings */
#define C_BINDINGS_COUNT 3

/* Number of user-level application keybindings */
#define BINDINGS_COUNT 14

/**
 * MODKEY - The modifier key required for all keybindings
 *
 * This key must be held down for any keybinding to trigger. Setting this to
 * WLR_MODIFIER_ALT means taht all keybindings require Alt to be pressed.
 */
#define MODKEY WLR_MODIFIER_ALT

/**
 * compositor_binding - Binds a key to a compositor function
 * @key: The xkb keysym that triggers this binding
 * @fptr: Function pointer to the compositor action to execute
 *
 * Used for actions that directly control the compositor, like:
 * - Cycling between windows
 * - Closing the focused window
 * - Terminating the compositor
 *
 * The function pointer receives the server state as its parameter, allowing it
 * to manipulate compositor-level state.
 */
typedef struct {
  xkb_keysym_t key;
  void (*fptr)(struct tinywl_server *server);
} compositor_binding;

/**
 * user_binding - Binds a key to a shell command
 * @key: The xkb keysym that triggers this binding
 * @command: Shell command string to execute
 *
 * Used for launching external applications.
 *
 * The command inherits the compositor's environment, including WAYLAND_DISPLAY,
 * so launched applications connect to this compositor.
 */
typedef struct {
  xkb_keysym_t key;
  char *command;
} user_binding;

/**
 * get_c_bindings - Returns array of compositor keybindings
 *
 * Return: Pointer to static array of compositor_binding structs
 */
const compositor_binding *get_c_bindings(void);

/**
 * get_bindings - Returns array of user keybindings
 *
 * Return: Pointer to static array of user_binding structs
 */
const user_binding *get_bindings(void);

#endif
