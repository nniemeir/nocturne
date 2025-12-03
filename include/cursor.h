/**
 * cursor.h
 *
 * Cursor management and pointer event handling.
 *
 * OVERVIEW:
 * Handles all aspects of the cursor/pointer in the compositor.
 *
 * CURSOR MODES:
 * - PASSTHROUGH: Normal mode where pointer events go to clients
 * - MOVE: User is dragging a window around the screen
 * - RESIZE: User is resizing a window by dragging an edge/corner
 *
 * These modes are defined in server.h as tiny_wl_cursor_mode enum because the
 * tinywl_server struct depends on them and the cursor functions depend on it.
 *
 * POINTER EVENTS:
 * - Motion: Mouse movement (relative deltas)
 * - Motion Absolute: Direct position
 * - Button: Mouse button press/release
 * - Axis: Scroll wheel or touchpad scrolling
 * - Frame: Event grouping marker
 *
 * INTERACTIVE OPERATIONS:
 * When a user wants to move or resize a window:
 * - begin_interactive() sets the mode and records initial state
 * - Cursor motion events are intercepted by the compositor
 * - process_cursor_move() or process_cursor_resize() handles the motion
 * - On button release, reset_cursor_mode() returns to normal
 */

#ifndef CURSOR_H
#define CURSOR_H

#include "server.h"

/**
 * reset_cursor_mode - Returns cursor to passthrough mode
 * @server: Server state structure
 *
 * Called when an interactive operation (move/resize) completes. This restores
 * normal cursor behavior where pointer events are sent to clients instead of
 * being consumed by the compositor.
 *
 * Clears:
 * - cursor_mode: Set back to TINYWL_CURSOR_PASSTHROUGH
 * - grabbed_toplevel: Cleared to NULL
 */
void reset_cursor_mode(struct tinywl_server *server);

/**
 * process_cursor_move - Handles cursor motion during window move
 * @server: Server state structure
 *
 * Called on every pointer motion event while in MOVE mode. Calculates the new
 * window position based on:
 * - Current cursor position
 * - Initial grab offset (where on the window the user grabbed)
 *
 * The window follows the cursor maintaining the same grab point.
 */
void process_cursor_move(struct tinywl_server *server);

/**
 * process_cursor_resize - Handles cursor motion during window resize
 * @server: Server state structure
 *
 * Called on every pointer motion event while in RESIZE mode. More complex than
 * move because:
 * - Can resize from any edge or corner
 * - May need to move the window (resizing from top/left)
 * - Must respect minimum size
 *
 * Uses server->resize_edges to know which edges are being dragged.
 */
void process_cursor_resize(struct tinywl_server *server);

/**
 * process_cursor_motion - Main cursor motion handler
 * @server: Server state structure
 * @time: Timestamp of the motion event in milliseconds
 *
 * Called for every cursor motion event. Dispatches based on cursor mode.
 *
 * In passthrough mode, also handles:
 * - Updating cursor image (default arrow when over empty space)
 * - Sending pointer enter/leave events to surfaces
 * - Updating which surface has pointer focus
 */
void process_cursor_motion(struct tinywl_server *server, uint32_t time);

/**
 * server_cursor_button - Handles mouse button press/release
 * @listener: Wayland listener that triggered this callback
 * @data: Pointer to wlr_pointer_button_event
 *
 * Called when any mouse button is pressed or released. Handles:
 * - Forwarding button events to focused client
 * - Ending interactive move/resize on button release
 * - Focusing windows on button press
 *
 * BUTTON STATES:
 * - WL_POINTER_BUTTON_STATE_PRESSED: Button pressed down
 * - WL_POINTER_BUTTON_STATE_RELEASED: Button released
 */
void server_cursor_button(struct wl_listener *listener, void *data);

/**
 * server_cursor_motion - Handles relative pointer motion
 * @listener: Wayland listener that triggered this callback
 * @data: Pointer to wlr_pointer_motion_event
 *
 * Called when a pointer device reports relative motion (delata movement). This
 * is the normal case for mice and trackpads.
 *
 * The cursor automatically:
 * - Constrains motion to output boundaries
 * - Applies acceleration curves from libinput
 * - Handles multiple pointer devices
 */
void server_cursor_motion(struct wl_listener *listener, void *data);

/**
 * server_cursor_motion_absolute - Handles absolute pointer motion
 * @listener: Wayland listener that triggered this callback
 * @data: Pointer to wlr_pointer_motion_absolute_event
 *
 * Called when a pointer device reports absolute position, this happens when:
 * - Running nested under Wayland/X11 (you move into the window)
 * - Using graphics tablets
 * - Using touch screens in pointer emulation mode
 *
 * The compositor warps the cursor to the absolute position.
 */
void server_cursor_motion_absolute(struct wl_listener *listener, void *data);

/**
 * server_cursor_axis - Handles scroll wheel and touchpad scroll
 * @listener: Wayland listener that triggered this callback
 * @data: Pointer to wlr_pointer_axis_event
 *
 * Called for scroll events:
 * - Mouse whell (vertical and horizontal)
 * - Touchpad two-finger scroll
 * - Trackpoint scroll
 *
 * Event provides:
 * - Orientation: Vertical or horizontal
 * - Delta: Continuous scroll amount
 * - Discrete: Click/notch count (for mouse wheels)
 * - Source: What type of device generated the event
 */
void server_cursor_axis(struct wl_listener *listener, void *data);

/**
 * server_cursor_frame - Signals end of pointer event group
 * @listener: Wayland listener that triggered this callback
 * @data: Unused
 *
 * Frame events group related pointer events together. For example:
 * - Two axis events from X and Y scroll happen together
 * - Motion + button press that occur simultaneously
 *
 * Clients should process accumulated events when they receive a frame.
 * This helps with smooth scrolling and multi-axis input.
 */
void server_cursor_frame(struct wl_listener *listener, void *data);

/**
 * begin_interactive - Starts interactive window move or resize
 * @toplevel: The window to operate on
 * @mode: TINYWL_CURSOR_MOVE or TINYWL_CURSOR_RESIZE
 * @edges: For resize, which edges are being dragged (WLR_EDGE_*)
 *
 * Sets up an interactive operation where the compositor intercepts pointer
 * events to move or resize a window. Records initial state:
 * - Cursor position relative to window
 * - Window geometry at start of operation
 * - Which edges are being resized (for resize mode)
 *
 * After calling this, pointer events are consumed by process_cursor_move() or
 * process_cursor_resize() instead of being sent to clients.
 */
void begin_interactive(struct tinywl_toplevel *toplevel,
                       enum tinywl_cursor_mode mode, uint32_t edges);

#endif
