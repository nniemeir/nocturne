/**
 * toplevel.h
 *
 * Top-level window management.
 *
 * OVERVIEW:
 * A toplevel is what XDG Shell calls a main application window. Toplevels are
 * different than surfaces:
 * - wl_surface: A basic rectangle that can be drawn to
 * - xdg_surface: A surface with desktop-window behavior (XDG Shell)
 * - xdg_toplevel: An xdg_surface that is a main window (not a popup)
 * - tinywl_toplevel: The compositor's tracking struct for an xdg_toplevel
 *
 * WINDOW LIFECYCLE:
 * 1. Client creates wl_surface
 * 2. Client creates xdg_surface from the wl_surface
 * 3. Client creates xdg_toplevel from the xdg_surface
 * 4. server_new_xdg_toplevel() is called
 * 5. Client commits initial state
 * 6. Compositor sends configure event
 * 7. Client acknowledges configure
 * 8. Client attaches first buffer and commits
 * 9. Window is now visible and can be used
 * 10. User interacts with window
 * 11. Client detaches buffer
 * 12. Client destroys xdg_toplevel
 * 13. Compositor cleans up tinywl_toplevel
 *
 * WINDOW STATES:
 * XDG Shell defines several window states:
 * - Normal: Regular window, can be moved and resized
 * - Maximized: Fills the entire output (minus panels/bars)
 * - Fullscreen: Fills the entire output
 * - Minimzed: Hidden but not destroyed
 * - Activatived: Has keybaord focus
 * - Tiled: Part of a tiling layout
 *
 * WINDOW GEOMETRY:
 * The window geometry is the visible bounds of the window. We use this
 * information for window positioning, resizing, and determining where to put
 * popups.
 *
 * WINDOW PROPERTIES:
 * - Title
 * - App ID
 * - Min/max size
 * - Parent
 *
 * INTERACTIVE OPERATIONS:
 * User can request interactive operations:
 * - Move: Drag the window around
 * - Resize: Drag edges/corners to resize
 * - Show window menu: Right-click on titlebar
 *
 * These operations are request, the compositor can ignore them.
 */

#ifndef TOPLEVEL_H
#define TOPLEVEL_H

#include <wayland-server-core.h>

#include "server.h"

/**
 * struct tinywl_toplevel - Represents a top-level window (application window)
 * @link: List node for server->toplevels list
 * @server: Back-pointer to the compositor server
 * @xdg_toplevel: The underlying wlroots xdg_toplevel object
 * @scene_tree: Scene graph node for this window
 * @border_top: Scene rectangle for top border
 * @border_bottom: Scene rectangle for bottom border
 * @border_left: Scene rectangle for left border
 * @border_right: Scene rectangle for right border
 * @map: Listener for surface map event (window becomes visible)
 * @unmap: Listener for surface unmap event (window becomes invisible)
 * @commit: Listener for surface commit event (new state committed)
 * @destroy: Listener for toplevel destruction
 * @request_move: Listener for move requests from client
 * @request_resize: Listener for resize requests from client
 * @request_maximize: Listener for maximize requests from client
 * @request_fullscreen: Listener for fullscreen requests from client
 *
 * Each application window gets one of these structs. It tracks:
 * - The xdg_toplevel (contains window properties, state, geometry)
 * - Scene graph integration (where in the scene tree it is)
 * - Window borders (visual decoration)
 * - Event listeners for window lifecycle and requests
 *
 * SCENE TREE:
 * Each window has its own scene tree node. This node:
 * - Contains the window's surface
 * - Contains the border decorations
 * - Can be moved/raised/lowered in the scene
 * - Automatically clips to output boundaries
 *
 * The scene tree makes rendering automatic, we update the tree structure and
 * wlroots figures out what needs to be redrawn.
 *
 * BORDERS:
 * Nocturne draws server-side borders (decorations) around windows. The color
 * and width of these borders will be configurable in the future.
 *
 * XDG Decoration protocol lets clients and compositor negotiate which to use
 *
 * LIFECYCLE EVENTS:
 * Map/unmap control visibility. An unmapped window exists but isn't visible,
 * while a mapped window is visible.
 *
 * A window can be mapped and unmapped multiple times before being destroyed.
 *
 * REQUEST EVENTS:
 * Clients can request operations, this is the general pattern:
 * 1. User clicks titlebar button
 * 2. Client sends request to compositor
 * 3. Compositor decides whether to honor it
 * 4. If honored, compositor sends configure event
 * 5. Client updates its rendering
 * 6. Client commits new buffer
 *
 * The serial number in requests helps verify that the requests are in response
 * to user input, rather than a malicious client trying to interfere with window
 * management.
 */
struct tinywl_toplevel {
  /* Linked list node. All toplevels are in server->toplevels */
  struct wl_list link;

  /* Back-pointer to the server that this window belongs to */
  struct tinywl_server *server;

  /* The underlying wlroots xdg_toplevel object */
  struct wlr_xdg_toplevel *xdg_toplevel;

  /* Scene graph node for this window and its decorations */
  struct wlr_scene_tree *scene_tree;

  /* Border decorations (server-side) */
  struct wlr_scene_rect *border_top;
  struct wlr_scene_rect *border_bottom;
  struct wlr_scene_rect *border_left;
  struct wlr_scene_rect *border_right;

  /* Lifecycle event listeners */
  struct wl_listener map;     /* Window becomes visible*/
  struct wl_listener unmap;   /* Window becomes invisible */
  struct wl_listener commit;  /* New surface state committed */
  struct wl_listener destroy; /* Window is being destroyed */

  /* Client request listeners */
  struct wl_listener request_move;       /* Client wants to move window */
  struct wl_listener request_resize;     /* Client wants to resize window */
  struct wl_listener request_maximize;   /* Client wants to maximize */
  struct wl_listener request_fullscreen; /* Clients wants fullscreen */
};

/**
 * server_new_xdg_toplevel - Handles new toplevel window creation
 * @listener: Wayland listener that triggered this callback
 * @data: Pointer to wlr_xdg_toplevel
 *
 * Called when a client creates a new toplevel (applicaiton window).
 * - Allocates tinywl_toplevel struct to track window state and listeners
 * - Creates scene tree node
 * - Creates border decorations
 * - Registers event listeners
 *
 * After this function returns, the client:
 * - Commits initial state (without a buffer)
 * - Wait for configure event
 * - Render at the size we suggest
 * - Attach buffer and commit
 * - Map event fires
 *
 * We send configure with size (0,0) to let the client pick its own size, a
 * tiling compositor would send a specific size to make the window tile.
 */
void server_new_xdg_toplevel(struct wl_listener *listener, void *data);

#endif
