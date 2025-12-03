/**
 * utils.h
 *
 * Utility functions for common compositor operations.
 *
 * OVERVIEW:
 * This module provides helper functions that are used throughout the compositor
 * for common tasks like:
 * - Launching external programs
 * - Managing window focus
 * - Finding windows at screen positions
 * - Closing windows
 * - Window cycling
 * - Terminating the compositor
 *
 * These functions implement higher-level compositor logic that builds on the
 * lower-level wlroots primatives.
 */

#ifndef UTILS_H
#define UTILS_H

#include "server.h"

/**
 * execute_program - Launches an external program
 * @name: Command to execute (passed to /bin/sh -c)
 *
 * Forks and executes a shell command. Used for keybindings that launch
 * applications. The command runs in the compositor's environment, which allows
 * the launched program to connect to the compositor because it has access to
 * WAYLAND_DISPLAY.
 *
 * FORK AND EXEC:
 * - fork() creates a copy of the compositor process
 * - Child process calls execl() to replace itself with the new program
 * - Parent (compositor) keeps running
 *
 * The forked child inherits:
 * - Environment variables
 * - File descriptors
 * - Current directory
 *
 * SHELL EXECUTION:
 * Since we execute through /bin/sh, the command can use shell syntax. This
 * includes:
 * - Pipes
 * - Redirects
 * - Environment variables
 * - Shell builtins
 *
 * This function will be modified in the future to use wait() for child
 * processes as to prevent zombie processes.
 */
void execute_program(char *name);

/**
 * focus_toplevel - Sets keyboard focus to a window
 * @toplevel: The window to focus (or NULL to clear focus)
 *
 * Focuses a window, giving it keyboard input. This function:
 * - Checks if already focused
 * - Deactivates previously focused window
 * - Raises the new window to the top
 * - Moves window to front of toplevels list
 * - Activates the new window
 * - Tells seat to send keyboard input to this window
 *
 * Keyboard and pointer focus are independent of each other.
 *
 * ACTIVATION:
 * wlr_xdg_toplevel_set_activated() tells the client whether it's active.
 * Clients use this to:
 * - Change titlebar color (active vs inactive)
 * - Show/hide blinking cursor
 * - Dim content when inactive
 *
 * Z-ORDER:
 * wlr_scene_node_raise_to_top() moves the window's scene node to the top of the
 * stack. Windows are rendered in tree order (bottom to top), so moving to top
 * makes the window render on top of others.
 *
 * LIST ORDER:
 * We also move the window to the front of server->toplevels. This list is used
 * for window cycling, most recently focused windows are at the front and least
 * recently at the back.
 */
void focus_toplevel(struct tinywl_toplevel *toplevel);

/**
 * desktop_toplevel_at - Find window at screen coordiantes
 * @server: Server state structure
 * @lx: X coordinate in layout space
 * @ly: Y coordinate in layout space
 * @surface: Output parameter for the surface at this position
 * @sx: Output parameter for surface-relative X coordinate
 * @sy: Output parameter for surface-relative Y coordinate
 *
 * Finds the topmost window at the given screen coordinates.
 *
 * COORDIANTE SPACES:
 * - Layout coordinates: Global desktop space
 * - Surface coordinates: Relative to top-left of surface
 *
 * This function translates from layout to surface coordinates.
 *
 * SCENE GRAPH QUERY:
 * Uses wlr_scene_node_at() to walkt the scene graph and find the topmost buffer
 * node at the given position. The scene graph handles:
 * - Z-ordering
 * - Clipping (windows outside outputs are skipped)
 * - Transformations (rotations, scaling)
 *
 * We then walk up the scene tree to find the tinywl_topolevel that owns this
 * surface. This is done by checking node->data fields until we find one that's
 * non-NULL (which we set to point to tinywl_toplevel)
 *
 * Return: Pointer to toplevel at this position, or NULL if none
 */
struct tinywl_toplevel *desktop_toplevel_at(struct tinywl_server *server,
                                            double lx, double ly,
                                            struct wlr_surface **surface,
                                            double *sx, double *sy);

/**
 * close_focused_surface - Closes the window with keyboard focus
 * @server: Server state structure
 *
 * Forcefully closes the window that currently has keyboard focus by sending
 * SIGTERM to its process.
 *
 * WAYLAND CLOSE:
 * There's no close message in Wayland. Windows close themselves when:
 * - User clicks close button (client handles this)
 * - Client crashes or exits
 * - Compositor kills the client's connection
 *
 * We implement this by finding the client's PID and sending SIGTERM.
 *
 * FINDING THE CLIENT:
 * 1. Get surface with keyboard focus from seat
 * 2. Get wl_client that owns the surface
 * 3. Get client's PID via wl_client_get_credentials()
 * 4. Send SIGTERM to that PID
 *
 * Sending SIGTERM like this is not the best approach for a few reasons:
 * - Clients might not exit on SIGTERM
 * - Some clients run in sandboxes so the PID wouldn't match
 *
 * In the future, this will likely be changed to destroy the wl_client
 * connection instead so that the client will clean up it's resources before
 * exiting.
 */
void close_focused_surface(struct tinywl_server *server);

/**
 * cycle_toplevel - Switches focus to the next window (like Alt+Tab)
 * @server: Server state structure
 *
 * Implements window cycling. Moves focus to the next window in the toplevels
 * list. Since focus_toplevel() moves windows to the front of the list, this
 * cycles through window in most recently used order.
 *
 * This only does something if there are two or more windows.
 */
void cycle_toplevel(struct tinywl_server *server);

/**
 * terminate_display - Shuts down the compositor
 * @server: Server state structure
 *
 * Signals the Wayland event loop to exit. This causes wl_display_run() to
 * return, which leads to clenaup and exit.
 *
 * This is what gets called by the quit keybinding (Alt+Escape by default)
 */
void terminate_display(struct tinywl_server *server);

#endif
