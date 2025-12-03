/**
 * output.h
 *
 * Display/monitor management and rendering.
 *
 * OVERVIEW:
 * An "output" in Wayland terminology is a display or monitor. This module
 * handles outputs being connected, disconnected, and rendering frames to them.
 *
 * OUTPUTS VS MONITORS:
 * - Physical monitor: The actual hardware screen
 * - wlr_output: wlroots' representation of a display
 * - tinywl_output: Our compositor's tracking struct for an output
 *
 * LIFECYCLE:
 * - Monitor is connected (or compositor starts with monitors connected)
 * - Backend emits new_output event
 * - server_new_output() creates tinywl_output and configures the output
 * - Output is added to output layout
 * - Frame events trigger rendering at the monitor's refresh rate
 * - Monitor is disconnected, destroy event, cleanup
 *
 * OUTPUT CONFIGURATION:
 * When an output is detected, we configure:
 * - Mode: Resolution and refresh rate (e.g., 1920x1080@60Hz)
 * - Enabled state: Turn the output on
 * - Position in layout: Where this monitor sits in the virtual desktop
 * - Scale: For HiDPI displays (1x, 2x, etc)
 * - Transform: Rotation (normal, 90, 180, 270)
 *
 * We pick the preferred mode on startup, which is usually the native resolution
 * of the display and the highest refresh rate supported at that resolution.
 *
 * FRAME EVENTS:
 * The output emits a "frame" event when it's ready to display a new image.
 * This typically happens at the monitor's refresh rate.
 *
 * On each frame event, we:
 * - Render the scene graph to the output
 * - Commit the output state (send it to the display)
 * - Send frame_done events to clients (so they know to draw next frame)
 */

#ifndef OUTPUT_H
#define OUTPUT_H

#include <wayland-server-core.h>

#include "server.h"

/**
 * struct tinywl_output - Represents a single display/output
 * @link: List node for server->outputs list
 * @server: Back-pointer to the compositor server
 * @wlr_output: The underlying wlroots output object
 * @frame: Listener for frame events (time to render)
 * @request_state: Listener for state change requests from backend
 * @destroy: Listener for output disconnect events
 *
 * Each connected monitor gets one of these structs. It tracks:
 * - The wlroots output object (handles hardware interaction)
 * - Event listeners for rendering and lifecycle
 * - Link to the server that owns this output
 *
 * FRAME EVENT:
 * The frame event is emitted when the output is ready for a new frame, this is
 * our signal to render and typically fires at the monitor's refresh rate.
 *
 * REQUEST_STATE EVENT:
 * Some backends (Wayland, X11) can request state changes.
 *
 * DESTROY EVENT:
 * Emitted when the output is connected. We must clean up the tinywl_output
 * struct and remove it from the server's output list.
 */
struct tinywl_output {
  /* Linked list node. All outputs are in server->outputs list*/
  struct wl_list link;

  /* Back-pointer to the server this output belongs to */
  struct tinywl_server *server;

  /* The underlying wlroots output object */
  struct wlr_output *wlr_output;

  /* Event listeners */
  struct wl_listener frame;         /* Called at refresh rate to render */
  struct wl_listener request_state; /* Backend requests state change */
  struct wl_listener destroy;       /* Output was disconnected */
};

/**
 * server_new_output - Handles new output (monitor) being connected
 * @listener: Wayland listener that triggered this callback
 * @data: Pointer to wlr_output
 *
 * Called when a monitor is connected or when the compositor starts with
 * monitors already connected. This does a lot:
 *
 * 1. Initializes rendering for the output
 *    - Associates the output with our allocator and renderer
 *
 * 2. Enables the output
 *    - Some outputs start disabled, we turn them on
 *
 * 3. Sets a mode
 *    - Chooses resolution and refresh rate
 *    - Uses the monitor's preferred mode
 *    - Some backends (headless, wayland nested) don't have modes
 *
 * 4. Commits the initial state
 *    - Applies enabled state and mode
 *
 * 5. Allocates tinywl_output truct
 *    - Tracks this output's state and listeners
 *
 * 6. Registers event listeners
 *    - frame: Called when ready to render next frame
 *    - request_state: Backend wants to change output state
 *    - destroy: Output was disconnected
 *
 * 7. Adds to output layout
 *    - wlr_output_layout_add_auto() arranges monitors left to right
 *    - Creates wl_output global that clients can see
 *    - Allows clients to query DPI, scale, manufacturer, etc.
 *
 * 8. Creates scene output
 *    - Connects output to scene graph for rendering
 *    - Scene graph automatically handles rendering to this output
 *
 * AUTOMATIC LAYOUT:
 * We use a simple approach with add_auto, which arranges monitors in the order
 * they're detected.
 */
void server_new_output(struct wl_listener *listener, void *data);

#endif
