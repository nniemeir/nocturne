/**
 * server.h
 *
 * Core compositor state and structures.
 *
 * OVERVIEW:
 * This header defines the central data structures that represent the
 * compositor's state. The tinywl_server struct contains everything that the
 * compositor needs to function:
 * - Wayland display and event loop
 * - Hardware backend (DRM, Wayland, X11, etc.)
 * - Rendering system
 * - Scene graph
 * - Input devices and cursor
 * - Outputs (monitors)
 * - Windows (toplevels)
 * - Shell protocol handlers
 *
 * ARCHITECTURE:
 * The compositor is event-driven. The main thread runs an event loop that waits
 for events:
 * - Client messages
 * - Input events
 * - DIsplay events
 * - Timers and callbacks
 *
 * When an event occurs, the corresponding listener's notify function is called.
 * That function can access the server state through the listener's parent
 * struct (using wl_container_of).
 *
 * THREADING:
 * wlroots and libwayland are single-threaded. All compositor code runs on the
 * main thread. The event loop uses epoll to efficiently wait for events.
 *
 * CURSOR MODES:
 * The cursor can be in different modes depending on what the user is doing,
 * this affects how cursor motion is processed.
 */

#ifndef SERVER_H
#define SERVER_H

#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>

/**
 * enum tinywl_cursor_mode - Cursor interaction modes
 * @TINYWL_CURSOR_PASSTHROUGH: Normal mode - events go to clients
 * @TINYWL_CURSOR_MOVE: User is dragging a window
 * @TINYWL_CURSOR_RESIZE: User is resizing a window
 *
 * The cursor mode determines how pointer motion events are processed.
 */
enum tinywl_cursor_mode {
  TINYWL_CURSOR_PASSTHROUGH, // Normal mode - pointer events go to clients
  TINYWL_CURSOR_MOVE,        // Dragging a window
  TINYWL_CURSOR_RESIZE,      // Resizing a window
};

/**
 * struct tinywl_server - Core state structure for the compositor
 *
 * This structure holds all the major components needed to run the compositor:
 * - The Wayland display server
 * - Hardware backend for I/O
 * - Rendering and scene management for compositing windows
 * - Input handling (cursor, keyboards, seat)
 * - Lists of outputs (monitors), toplevels (windows), etc.
 */

struct tinywl_server {
  /* Core Wayland/wlroots objects */
  struct wl_display *wl_display;                /* Wayland display sevrer*/
  struct wlr_backend *backend;                  /* Hardware backend*/
  struct wlr_renderer *renderer;                /* Rendering system*/
  struct wlr_allocator *allocator;              /* Buffer allocator */
  struct wlr_scene *scene;                      /* Scene graph root */
  struct wlr_scene_output_layout *scene_layout; /* Scene + layout */

  /* XDG Shell - Protocol for application windows */
  struct wlr_xdg_shell *xdg_shell;
  struct wl_listener new_xdg_toplevel; /* New window created*/
  struct wl_listener new_xdg_popup;    /* New popup created */
  struct wl_list toplevels;            /* List of all windows */

  /* Cursor/pointer handling */
  struct wlr_cursor *cursor;                 /* Logical cursor */
  struct wlr_xcursor_manager *cursor_mgr;    /* Cursor themes */
  struct wl_listener cursor_motion;          /* Relative Motion */
  struct wl_listener cursor_motion_absolute; /* Absolute motion */
  struct wl_listener cursor_button;          /* Button press/release */
  struct wl_listener cursor_axis;            /* Scroll wheel */
  struct wl_listener cursor_frame;           /* Event batching*/

  /* Input seat */
  struct wlr_seat *seat;                    /* Input seat */
  struct wl_listener new_input;             /* New input device */
  struct wl_listener request_cursor;        /* Client cursor request */
  struct wl_listener request_set_selection; /* Clipboard request */
  struct wl_list keyboards;                 /* List of keyboards*/

  /* Interactive move/resize state */
  enum tinywl_cursor_mode cursor_mode;      /* Current interaction mode*/
  struct tinywl_toplevel *grabbed_toplevel; /* Window being moved/resized */
  double grab_x, grab_y;                    /* Cursor offset at grab start */
  struct wlr_box grab_geobox;               /* Window geometry at grab start */
  uint32_t resize_edges;                    /* Which edges being resized */

  /* Output (monitor) handling */
  struct wlr_output_layout *output_layout; /* Output arrangement*/
  struct wl_list outputs;                  /* List of outputs */
  struct wl_listener new_output;           /* New output connected */
};

/**
 * server_cleanup - Clean up all compositor resources
 * @server: Server state structure
 *
 * Called on compositor shutdown. Frees all allocated resources:
 * - Disconnects all clients
 * - Removes all event listeners
 * - Destroys scene graph
 * - Destroys cursor and cursor manager
 * - Destroys allocator and renderer
 * - Destroys backend
 * - Destroys Wayland display
 *
 * This should be called on exit.
 */
void server_cleanup(struct tinywl_server *server);

#endif
