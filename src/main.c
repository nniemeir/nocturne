/**
 * main.c
 *
 * TinyWL-based wlroots compositor.
 *
 * OVERVIEW:
 * This is a minimal Wayland compositor built using the wlroots library. A
 * compositor is the core component in a Wayland desktop environment. It manages
 * displays, handles input from keyboards and mice, arranges windows (surfaces
 * provided by client applications), and composites (renders) everything onto
 * the screen.
 *
 * wlroots is a library that abstracts away many low-level details that come
 * with writing a compositor, including:
 * - Backend abstraction (DRM/KMS, X11, Wayland nested, headless)
 * - Renderer abstraction (OpenGL ES, Vulkan, Pixman)
 * - Scene graph for efficient rendering
 * - Input device management
 * - Output (monitor) management
 * - Protocol implementations (XDG Shell, layer shell, etc.)
 *
 * The compositor that we are starting from, TinyWL, is considered the Minimum
 * Viable Product for a wlroots-based compositor. It supports basic window
 * management: opening, moving, resizing, and closing windows; keyboard and
 * mouse input; and multi-monitor setups. It uses XDG Shell for window
 * management, which is a standard Wayland protocol extension for desktop-style
 * surfaces.
 *
 * The program initializes the Wayland display, sets up the backend for hardware
 * access, creates a renderer for drawing, and enters an event loop to handle
 * client connections, input events, and rendering.
 */

#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
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

#include "cursor.h"
#include "input.h"
#include "output.h"
#include "popup.h"
#include "server.h"
#include "toplevel.h"

/*
 * Must be included for xwayland support.
 * XWayland allows running legacy X11 applications within a Wayland compositor
 * by embedded an X11 server.
 * Currently hooked up but not fully functional.
 */
#include <wlr/xwayland.h>

/**
 * process_args - Parse command-line arguments
 * @argc: Argument count
 * @argv: Argument array
 * @startup_cmd: Output parameter for startup command string
 *
 * Processes CLI arguments using getopt(), the POSIX standard for doing so
 *
 * OPTIONS:
 * -h: Display program usage
 * -s <command>: Command to run after compositor starts
 *
 * Return: 1 to continue initialization, 0 to exit successfully, -1 on error
 */
int process_args(int argc, char *argv[], char **startup_cmd) {
  int c;
  while ((c = getopt(argc, argv, "s:h")) != -1) {
    switch (c) {
    case 'h':
      printf("Usage: %s [-s startup command]\n", argv[0]);
      return 0;
    case 's':
      *startup_cmd = optarg;
      break;
      /*
       * getopt returns '?' if it encounters an unknown option (e.g, if we tried
       *  using -q without including it in shortopts). optopt is the actual
       *  character provided.
       */
    case '?':
      fprintf(stderr, "Unknown option '-%c'. Run with -h for options.\n",
              optopt);
      return -1;
    }
    /* Check for extra options that aren't associated with any flag */
    if (optind < argc) {
      printf("Usage: %s [-s startup command]\n", argv[0]);
      return -1;
    }
  }
  return 1;
}

/**
 * setup_shell_and_input - Initialize XDG shell and input handling
 * @server: Server state structure
 *
 * Sets up the window management protocol (XDG Shell) and input handling. This
 * is where we configure how the compositor will interact with client
 * applications and handle user input.
 *
 * XDG SHELL:
 * XDG Shell is a Wayland protocol extension that defines how desktop
 * applications create and manage windows. It provides:
 * - Toplevels (normal application windows)
 * - Popups (menus, tooltips, dropdowns)
 * - Window states (maximized, fullscreen, minimized)
 * - Client-side decorations
 *
 * See Drew Devault's blog for more information on Wayland shells:
 * https://drewdevault.com/2018/07/29/Wayland-shells.html
 *
 * CURSOR HANDLING:
 * wlroots cursor abstraction provides:
 * - Hardware cursor support (GPU-rendered cursor)
 * - Multiple pointer device aggregation
 * - Scale factor handling for HiDPI
 * - Cursor theme loading (Xcursor format)
 *
 * SEAT:
 * A seat is a Wayland concept representing a set of input devices that belong
 * to a single user. A seat includes:
 * - One keyboard (or none)
 * - One pointer device (or none)
 * - Touch devices
 * - Drawing tablets
 *
 * Most systems have one seat (seat0), but multi-seat setups allow multiple
 * users to use the same computer simulatenously with separate keyboards, mice,
 * and monitors.
 *
 * Return: true on success, false on failure
 */
static bool setup_shell_and_input(struct tinywl_server *server) {
  /*
   * Initialize the list of toplevels (application windows).
   * wl_list is a circular doubly-linked list provided by Wayland.
   */
  wl_list_init(&server->toplevels);

  /*
   * Set up xdg-shell. The xdg-shell is a Wayland protocol which is
   * used for application windows.
   */
  server->xdg_shell = wlr_xdg_shell_create(server->wl_display, 3);

  /*
   * Register event listeners for new XDG shell surfaces.
   * These notify functions are called when clients create toplevels or popups.
   */
  server->new_xdg_toplevel.notify = server_new_xdg_toplevel;
  wl_signal_add(&server->xdg_shell->events.new_toplevel,
                &server->new_xdg_toplevel);
  server->new_xdg_popup.notify = server_new_xdg_popup;
  wl_signal_add(&server->xdg_shell->events.new_popup, &server->new_xdg_popup);

  /*
   * Creates a cursor, which is a wlroots utility for tracking the cursor
   * image shown on screen. The cursor:
   * - Aggregates input from multiple pointer devices
   * - Handles cursor themes and scaling
   * - Provides smooth cursor movement
   * - Constrains cursor to output boundaries
   */
  server->cursor = wlr_cursor_create();

  /*
   * Attach the cursor the output layout, this ensures that the cursor is
   * constrained to monitor boundaries, properly transitions between monitors,
   * and scales correctly on HiDPI displays.
   */
  wlr_cursor_attach_output_layout(server->cursor, server->output_layout);

  /*
   * Creates an xcursor manager, another wlroots utility which loads up
   * Xcursor themes to source cursor images from and makes sure that cursor
   * images are available at all scale factors on the screen (necessary for
   * HiDPI support).
   *
   * XCURSOR FORMAT:
   * Xcursor is the X11 cursor format, still used by Wayland for cursor themes.
   * NULL means use the default theme from environment variables or system
   * defaults. 24 is the cursor size in pixels.
   */
  server->cursor_mgr = wlr_xcursor_manager_create(NULL, 24);

  /*
   * wlr_cursor *only* displays an image on screen. It does not move around
   * when the pointer moves. However, we can attach input devices to it, and
   * it will generate aggregate events for all of them. In these events, we
   * can choose how we want to process them, forwarding them to clients and
   * moving the cursor around. More detail on this process is described in
   * https://drewdevault.com/2018/07/17/Input-handling-in-wlroots.html.
   *
   * CURSOR MODES:
   * We track three cursor modes:
   * - PASSTHROUGH: Normal mode, cursor events go to clients
   * - MOVE: User is dragging a window
   * - RESIZE: User is resizing a window
   */
  server->cursor_mode = TINYWL_CURSOR_PASSTHROUGH;

  /*
   * Register cursor event listeners.
   * Each event type has a corresponding handler function.
   *
   * HANDLER FUNCTION PROTOTYPE:
   * Wayland requires that handler functions follow this prototype:
   * void handler(struct wl_listener *listener, void *data);
   *
   * In handler functions that don't use both of these parameters, we cast the
   * unused parameter to void to surpress compiler warnings.
   */
  server->cursor_motion.notify = server_cursor_motion;
  wl_signal_add(&server->cursor->events.motion, &server->cursor_motion);
  server->cursor_motion_absolute.notify = server_cursor_motion_absolute;
  wl_signal_add(&server->cursor->events.motion_absolute,
                &server->cursor_motion_absolute);
  server->cursor_button.notify = server_cursor_button;
  wl_signal_add(&server->cursor->events.button, &server->cursor_button);
  server->cursor_axis.notify = server_cursor_axis;
  wl_signal_add(&server->cursor->events.axis, &server->cursor_axis);
  server->cursor_frame.notify = server_cursor_frame;
  wl_signal_add(&server->cursor->events.frame, &server->cursor_frame);

  /*
   * Configures a seat, which is a single "seat" at which a user sits and
   * operates the computer. This conceptually includes up to one keyboard,
   * pointer, touch, and drawing tablet device. We also rig up a listener to
   * let us know when new input devices are available on the backend.
   *
   * Clients use this to determine what input methods are available.
   */
  wl_list_init(&server->keyboards);
  server->new_input.notify = server_new_input;
  wl_signal_add(&server->backend->events.new_input, &server->new_input);
  server->seat = wlr_seat_create(server->wl_display, "seat0");

  /*
   * Register seat event listeners
   * request_set_cursor: Client wants to set cursor image
   * request_set_selection: Client wants to set clipboard contents
   */
  server->request_cursor.notify = seat_request_cursor;
  wl_signal_add(&server->seat->events.request_set_cursor,
                &server->request_cursor);
  server->request_set_selection.notify = seat_request_set_selection;
  wl_signal_add(&server->seat->events.request_set_selection,
                &server->request_set_selection);

  return true;
}

/**
 * finalize_startup - Complete server initialization and start acceptin clients
 * @server: Server state structure
 * @startup_cmd: Optional command to run on startup
 *
 * This finishes setting up the compositor:
 * - Creates a Wayland socket for client connections
 * - Starts the backend (enables displays and input devices)
 * - Sets WAYLAND_DISPLAY environment variable
 * - Executes startup command if provided
 *
 * WAYLAND SOCKET:
 * The Wayland display creates a Unix domain socket (typically at
 * /run/usr/1000/wayland-0) that clients connect to. The socket name is stored
 * in the WAYLAND_DISPLAY variable.
 *
 * BACKEND STARTUP:
 * Starting the backend:
 * - Opens DRM device files (/dev/dri/card0)
 * - Becomes the DRM master (exclusive access to GPU)
 * - Enumerates connected displays
 * - Sets up display modes
 * - Starts processing input events

 * Return: true on success, false on failure
 */
static bool finalize_startup(struct tinywl_server *server,
                             const char *startup_cmd) {
  /*
   * Add a Unix socket to the Wayland display.
   * wl_display_add_socket_auto() automatically chooses an available socket name
   * (wayland-0, wayland-1, etc.) by trying each in sequence.
   */
  const char *socket = wl_display_add_socket_auto(server->wl_display);
  if (!socket) {
    wlr_backend_destroy(server->backend);
    return false;
  }

  /*
   * Start the backend. After this, the compositor is live and handling
   * hardware.
   *
   * WHAT HAPPENS ON START:
   * - DRM backend: Opens GPU device, sets video modes, enables CRTCs
   * - Input backend: Opens input devices, starts reading events
   * - Wayland backend: Connects to parent compositor (for nested mode)
   * - X11 backend: Creates X11 window (for development/testing)
   */
  if (!wlr_backend_start(server->backend)) {
    wlr_backend_destroy(server->backend);
    wl_display_destroy(server->wl_display);
    return false;
  }

  /*
   * Set the WAYLAND_DISPLAY environment variable to our socket and run the
   * startup command if requested. Setting this varibale is important because
   * clients need to know which Wayland compositor to connect to. They check
   * this environment variable to find the socket path. By setting it before
   * spawning child processes, we ensure those processes inherit the variable
   * and can connect to us.
   */
  setenv("WAYLAND_DISPLAY", socket, true);
  if (startup_cmd) {
    /*
     * Fork and execute the startup command.
     * The child process inherits our environment, including WAYLAND_DISPLAY, so
     * it can connect to this compositor as a client.
     *
     * - fork() creates a copy of the current process
     * - Child process calls execl
     * - execl replaces the process with the new program
     * - Parent continues running the compositor
     *
     * Using /bin/sh -c allows the command to be a shell script with pipes,
     * redirects, etc., not just a single executable.
     */
    if (fork() == 0) {
      execl("/bin/sh", "/bin/sh", "-c", startup_cmd, (void *)NULL);
    }
  }
  wlr_log(WLR_INFO, "Running Wayland compositor on WAYLAND_DISPLAY=%s", socket);
  return true;
}

/**
 * setup_display_and_backend - Initialize Wayland display and hardware backend
 * @server: Server state structure
 *
 * Creates the fundamental compositor components:
 * - Wayland display (manages clients, globals, event loop)
 * - Backend (abstracts hardware access)
 *
 * WAYLAND DISPLAY:
 * The display is the core of the compositor. It:
 * - Manages the event loop (epoll-based)
 * - Accepts client connections
 * - Dispatches events to handlers
 * - Manages global objects (wl_compositor, wl_seat, etc.)
 *
 * BACKEND:
 * The backend abstracts hardware access. wlroots supports quite a few backends:
 * - DRM/KMS: Direct rendering on real hardware (production)
 * - Wayland: Nested compositor inside another compositor
 * - X11: Compositor inside an X11 window (development)
 * - Headless: No display, for testing (CI/automated testing)
 * - Multi: Combine multiple backends
 *
 * The autocreate function chooses the most suitable backend based on the
 * environment. If WAYLAND_DISPLAY is set, it uses Wayland backend. If DISPLAY
 * is set, it uses X11 backend. Otherwise, it tries DRM/KMS.
 *
 * Return: true on success, false on failure
 */
static bool setup_display_and_backend(struct tinywl_server *server) {
  /* The Wayland display is managed by libwayland. It handles accepting
   * clients from the Unix socket, manging Wayland globals, and so on. */
  server->wl_display = wl_display_create();

  /*
   * The backend is a wlroots feature which abstracts the underlying input and
   * output hardware. The autocreate option will choose the most suitable
   * backend based on the current environment, such as opening an X11 window
   * if an X11 server is running.
   *
   * wl_display_get_event_loop() returns the event loop that the backend will
   * use to schedule callbacks and handle I/O.
   */
  server->backend = wlr_backend_autocreate(
      wl_display_get_event_loop(server->wl_display), NULL);
  if (server->backend == NULL) {
    wlr_log(WLR_ERROR, "failed to create wlr_backend");
    return false;
  }
  return true;
}

/**
 * setup_rendering - Initialize renderer, allocator, and compositor globals
 * @server: Server state structure
 *
 * Sets up the rendering pipeline that composites all windows onto the screen,
 including:
 * - Renderer (draws pixels)
 * - Allocator (creates buffers)
 * - Compositor global (clients allocate surfaces)
 * - Scene graph (manages what to render)
 *
 * RENDERER:
 * The renderer abstracts the graphics API. wlroots supports:
 * - GLES2: OpenGL ES 2.0 (most compatible)
 * - Vulkan: Modern, efficient (best performance)
 * - Pixman: Software rendering (no GPU needed)
 *
 * The WLR_RENDERER environment variable can force a specific renderer
 *
 * ALLOCATOR:
 * The allocator creates GPU buffers for rendering. It's the bridge between the
 * renderer and the backend. Different backends need different buffer types:
 * - DRM: GBM (Generic Buffer Management)
 * - Wayland: wl_shm (shared memory) or dmabuf
 * - X11: SHM (shared memory)
 *
 * COMPOSITOR GLOBAL:
 * The wl_compositor global is how clients create surfaces. A surface is a
 * rectangular are that can be rendered to. Clients:
 * - Create a wl_surface
 * - Attach a buffer to it
 * - Commit the surface
 * - Compositor displays the buffer
 *
 * SCENE GRAPH:
 * The scene graph is a tree structure that represents what should be rendered
 * and where. It:
 * - Automatically tracks damage (what changed)
 * - Handles rendering order (Z-order)
 * - Manages buffer lifetimes
 * - Optimizes redraws (only redraw damaged regions)
 *
 * Return: true on success, false on failure
 */
static bool setup_rendering(struct tinywl_server *server) {
  /* Autocreates a renderer, either Pixman, GLES2 or Vulkan for us. The user
   * can also specify a renderer using the WLR_RENDERER env var.
   * The renderer is responsible for defining the various pixel formats it
   * supports for shared memory, this configures that for clients. */
  server->renderer = wlr_renderer_autocreate(server->backend);
  if (server->renderer == NULL) {
    wlr_log(WLR_ERROR, "failed to create wlr_renderer");
    return false;
  }

  /*
   * Initialize renderer with the Wayland display.
   * This creates the wl_shm global that clients use to create shared memory
   * buffers. It announces supported pixel formats.
   */
  wlr_renderer_init_wl_display(server->renderer, server->wl_display);

  /*
   * Autocreates an allocator for us.
   * The allocator is the bridge between the renderer and the backend. It
   * handles the buffer creation, allowing wlroots to render onto the
   * screen.
   *
   * BUFFER TYPES:
   * - DRM backend: GBM buffers (GPU memory)
   * - Wayland backend: wl_shm or dmabuf
   * - X11 backend: SHM (shared memory)
   *
   * The allocator automatically chooses the right type.
   */
  server->allocator =
      wlr_allocator_autocreate(server->backend, server->renderer);
  if (server->allocator == NULL) {
    wlr_log(WLR_ERROR, "failed to create wlr_allocator");
    return false;
  }

  /*
   * This creates some hands-off wlroots interfaces.
   *
   * COMPOSITOR:
   * The wlr_compositor global is necessary for clients to allocate surfaces.
   * Surfaces are rectangular areas that clients render to. The compositor
   * displays these surfaces on screen.
   *
   * SUBCOMPOSITOR:
   * The wl_subcompositor allows clients to create subsurfaces - surfaces that
   * are children of other surfaces. This is useful for:
   * - Video players (video surface + control overlay)
   * - Game engines (main view + UI overlay)
   * - Complex widget hierarchies
   *
   * DATA DEVICE MANAGER
   * Handles clipboard and drag-and-drop. Clients can't directly access the
   * clipboard - the compositor mediates all transfers to prevent security
   * issues.
   */
  struct wlr_compositor *compositor;
  compositor = wlr_compositor_create(server->wl_display, 5, server->renderer);
  wlr_subcompositor_create(server->wl_display);
  wlr_data_device_manager_create(server->wl_display);

  /*
   * Creates an output layout, which is a wlroots utility for working with an
   * arrangement of screens in a physical layout.
   *
   * OUTPUT LAYOUT:
   * This tracks:
   * - Physical arrangement of monitors
   * - Resolution and scale of each monitor
   * - Which monitor is the primary one
   *
   * It provides functiosn to:
   * - Convert between layout coordinates and monitor coordinates
   * - Determine which monitor contains a point
   * - Automatically arrange monitors
   */
  server->output_layout = wlr_output_layout_create(server->wl_display);

  /*
   * Configure a listener to be notified when new outputs are available on the
   * backend. This event fires when:
   * - A monitor is plugged in
   * - A monitor is turned on
   * - The compositor starts up (fired once per monitor)
   */
  wl_list_init(&server->outputs);
  server->new_output.notify = server_new_output;
  wl_signal_add(&server->backend->events.new_output, &server->new_output);

  /* Create a scene graph. This is a wlroots abstraction that handles all
   * rendering and damage tracking. All the compositor author needs to do
   * is add things that should be rendered to the scene graph at the proper
   * positions and then call wlr_scene_output_commit() to render a frame if
   * necessary.
   *
   * SCENE GRAPH BENEFITS:
   * - Automatic damage tracking (only redraw changed areas)
   * - Efficient rendering (skip offscreen/occluded content)
   * - Correct Z-ordering (windows stacked properly)
   * - Buffer management (when to release buffers)
   *
   * A scene graph is a tree. Each node can be:
   * - A buffer (actual pixels from a client)
   * - A rectangle (solid color, used for borders)
   * - A tree (container for other nodes)
   */
  server->scene = wlr_scene_create();
  server->scene_layout =
      wlr_scene_attach_output_layout(server->scene, server->output_layout);

  /*
   * Create XWayland server instance. This is a X11 server that runs inside the
   * Wayland compositor, allowing legacy X11 applications to run on Wayland.
   *
   * HOW XWAYLAND WORKS:
   * - XWayland provides an X11 server (Xwayland process)
   * - X11 apps connect to it like normal
   * - XWayland creates Wayland surfaces for X11 windows
   * - Compositor treats them like regular Wayland windows
   *
   * We've wired in xwayland to ensure it will start, but we will still need
   * to write client handling code for X11 windows to render in.
   */
  wlr_xwayland_create(server->wl_display, compositor, false);

  return true;
}

/**
 * main - Entry point for the program
 * @argc: Argument count
 * @argv: Argument array
 *
 * Handles CLI argument parsing, initialization of compositor, and cleanup.
 *
 * Orchestrates the initialization sequence:
 * - Initialize logging
 * - Parse command-line arguments
 * - Set up display and backend
 * - Set up rendering pipeline
 * - Set up input handling and XDG Shell
 * - Start accepting connections
 * - Run event loop until exit
 *
 * The event loop uses epoll to efficiently wait for events:
 * - Client messages (create window, destroy window, etc.)
 * - Input events (key press, mouse move, etc.)
 * - Display events (monitor connected, vblank, etc.)
 * - Timers and delayed callbacks
 *
 * The loop continues until:
 * - User presses quit keybding (MOD+Escape)
 * - Fatal error occurs
 * - wl_display_terminate() is called
 *
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE on failure
 */
int main(int argc, char *argv[]) {
  /*
   * Initialize wlroots logging.
   * WLR_DEBUG enables verbose logging, which is useful for development.
   * Other levels include: WLR_SILENT, WLR_ERROR, WLR_INFO
   *
   * NULL means log to stderr.
   */
  wlr_log_init(WLR_DEBUG, NULL);

  char *startup_cmd = NULL;

  /*
   * Parse command-line arguments.
   * This can set startup_cmd or return early on -h or error.
   */
  int args_result = process_args(argc, argv, &startup_cmd);

  if (args_result == -1) {
    exit(EXIT_FAILURE);
  } else if (args_result == 0) {
    exit(EXIT_SUCCESS);
  }

  /*
   * Initialize server structure with all zeros.
   * This ensures that all pointers are NULL and all integers are 0.
   */
  struct tinywl_server server = {0};

  /*
   * Initialize compositor in stages.
   * If any stage fails, we cleanup and exit.
   */
  if (!setup_display_and_backend(&server) || !setup_rendering(&server) ||
      !setup_shell_and_input(&server) ||
      !finalize_startup(&server, startup_cmd)) {
    server_cleanup(&server);
    exit(EXIT_FAILURE);
  }

  /*
   * Run the Wayland event loop. This does not return until you exit the
   * compositor. Starting the backend rigged up all of the necessary event
   * loop configuration to listen to libinput events, DRM events, generate
   * frame events at the refresh rate, and so on.
   *
   * EVENT LOOP:
   * The event loop uses epoll to efficiently wait for events:
   * - Call epoll_wait(): blocks until something happens
   * - Process all ready file descriptors
   * - Call any scheduled callbacks
   * - Repeat
   */
  wl_display_run(server.wl_display);

  /*
   * Cleanup all resources before exit.
   * This frees memory, closes file descriptors, etc.
   */
  server_cleanup(&server);

  exit(EXIT_SUCCESS);
}
