/**
 * input.h
 *
 * Input device management and seat handling.
 *
 * OVERVIEW:
 * This module handles input devices connecting to the compositor. The backend
 * (DRM, Wayland, X11) detects input devices and notifies us through the
 * new_input event. We then configure and register these devices with the seat.
 *
 * DEVICE TYPES:
 * wlroots supports several input device types:
 * - WLR_INPUT_DEVICE_KEYBOARD: Keyboards
 * - WLR_INPUT_DEVICE_POINTER: Mice, trackpads, trackpoints
 * - WLR_INPUT_DEVICE_TOUCH: Touch screens
 * - WLR_INPUT_DEVICE_TABLET_TOOL: Graphics tablets (stylus)
 * - WLR_INPUT_DEVICE_TABLET_PAD: Graphics tablet buttons/dials
 * - WLR_INPUT_DEVICE_SWITCH: Lid switches, tablet mode switches
 *
 * Currently Nocturne only handles keyboards and pointers.
 *
 * SEAT CONCEPT:
 * A "seat" represents a set of input devices for one user. It includes:
 * - At most one keyboard (but we can switch between multiple keyboards)
 * - A pointer (aggregated from all pointer devices)
 * - Touch devices
 * - Tablet devices
 *
 * The seat also tracks focus, which surface receives keyboard input and which
 * has the pointer over it.
 *
 * CAPABILITIES:
 * Clients query what input capabilities the seat has:
 * - WL_SEAT_CAPABILITY_POINTER: Seat has pointer/mouse
 * - WL_SEAT_CAPABILITY_KEYBOARD: Seat has keyboard
 * - WL_SEAT_CAPABILITY_TOUCH: Seat has touch
 *
 * We update these capabilities when devices are added/removed.
 *
 * SEAT REQUESTS:
 * Clients can make requests to the seat:
 * - request_set_cursor: Client wants to set cursor image
 * - request_set_selection: Client wants to set clipboard content
 */

#ifndef INPUT_H
#define INPUT_H

#include "server.h"

/**
 * server_new_pointer - Handles new pointer device
 * @server: Server state structure
 * @device: The input device that was connected
 *
 * Called when a pointer device is connected. We don't do much configuration
 * here, just attach the device to the cursor object.
 *
 * The cursor aggregates all pointer devices, so adding a second mouse just
 * works: both mice control the same cursor.
 */
void server_new_pointer(struct tinywl_server *server,
                        struct wlr_input_device *device);

/**
 * server_new_input - Handles new input device of any type
 * @listener: Wayland listener that triggered this callback
 * @data: Pointer to wlr_input_device
 *
 * This is the main entry point for input device hotplugging. Called when:
 * - Compositor starts and backend enumerates existing devices
 * - User plugs in a USB keyboard/moouse
 * - Bluetooth device connects
 *
 * Dispatches to specific handlers based on device type:
 * - Keyboard: server_new_keyboard()
 * - Pointer: server_new_pointer()
 *
 * After handling the device, this updates seat capabilities so that clients
 * know what input methods are available.
 */
void server_new_input(struct wl_listener *listener, void *data);

/**
 * seat_request_cursor - Handles client cursor image requests
 * @listener: Wayland listener that triggered this callback
 * @data: Pointer to wlr_seat_pointer_request_set_cursor_event
 *
 * Clients can request to change the cursor image, such as changing to a resize
 * cursor when hovering on window edges.
 */
void seat_request_cursor(struct wl_listener *listener, void *data);

/**
 * seat_request_set_selection - Handles clipboard set requests
 * @listener: Wayland listener that triggered this callback
 * @data: Pointer to wlr_seat_request_set_selection_event
 *
 * Called when a client wants to set the clipboard content, typically when the
 * user copies text or files.
 *
 * CLIPBOARD PROCESS:
 * - User presses Ctrl+C in an application 
 * - Application sends set_selection request with a data source
 * - Compositor stores the data source
 * - User presses Ctrl+V in another application
 * - That application requests data from the compositor
 * - Compositor asks the original application to provide the data
 * - Data is transferred to the requesting application
 *
 * The compositor mediates all clipboard transfers for security:
 * - Prevents apps from reading clipboard without user action
 * - Can filter or transform clipboard data
 * - Can restrict clipboard to within the same compositor session
 */
void seat_request_set_selection(struct wl_listener *listener, void *data);

/**
 *
 *
 *
 */
void reset_cursor_mode(struct tinywl_server *server);

/**
 *
 *
 *
 */
void process_cursor_move(struct tinywl_server *server);

/**
 *
 *
 *
 */
void process_cursor_resize(struct tinywl_server *server);

/**
 *
 *
 *
 */
void process_cursor_motion(struct tinywl_server *server, uint32_t time);

/**
 *
 *
 *
 */
void server_cursor_button(struct wl_listener *listener, void *data);

/**
 *
 *
 *
 */
void server_cursor_motion(struct wl_listener *listener, void *data);

/**
 *
 *
 *
 */
void server_cursor_motion_absolute(struct wl_listener *listener, void *data);

/**
 *
 *
 *
 */
void server_cursor_axis(struct wl_listener *listener, void *data);

/**
 *
 *
 *
 */
void server_cursor_frame(struct wl_listener *listener, void *data);

/**
 *
 *
 *
 */
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

/**
 *
 *
 *
 */
void server_new_keyboard(struct tinywl_server *server,
                         struct wlr_input_device *device);

#endif
