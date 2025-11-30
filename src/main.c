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
 * wlroots is a library that abstracts away low-level details like interacting
 * with graphics hardware (via backends like DRM/KMS for direct kernel access)
 * and input devices.
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
#include <signal.h>
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
#include "server.h"
#include "output.h"
#include "toplevel.h"
#include "popup.h"

/*
 * Must be included for xwayland support, currently hooked up but not functional
 */
#include <wlr/xwayland.h>

int process_args(int argc, char *argv[], char **startup_cmd) {
  int c;
  while ((c = getopt(argc, argv, "s:h")) != -1) {
    switch (c) {
    case 'h':
      printf("Usage: %s [-s startup command]\n", argv[0]);
      return 1;
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
    if (optind < argc) {
      printf("Usage: %s [-s startup command]\n", argv[0]);
      return 1;
    }
  }
  return 0;
}

static bool setup_shell_and_input(struct tinywl_server *server) {
  /* Set up xdg-shell version 3. The xdg-shell is a Wayland protocol which is
   * used for application windows. For more detail on shells, refer to
   * https://drewdevault.com/2018/07/29/Wayland-shells.html.
   */
  wl_list_init(&server->toplevels);
  server->xdg_shell = wlr_xdg_shell_create(server->wl_display, 3);
  server->new_xdg_toplevel.notify = server_new_xdg_toplevel;
  wl_signal_add(&server->xdg_shell->events.new_toplevel,
                &server->new_xdg_toplevel);
  server->new_xdg_popup.notify = server_new_xdg_popup;
  wl_signal_add(&server->xdg_shell->events.new_popup, &server->new_xdg_popup);

  /*
   * Creates a cursor, which is a wlroots utility for tracking the cursor
   * image shown on screen.
   */
  server->cursor = wlr_cursor_create();
  wlr_cursor_attach_output_layout(server->cursor, server->output_layout);

  /* Creates an xcursor manager, another wlroots utility which loads up
   * Xcursor themes to source cursor images from and makes sure that cursor
   * images are available at all scale factors on the screen (necessary for
   * HiDPI support). */
  server->cursor_mgr = wlr_xcursor_manager_create(NULL, 24);

  /*
   * wlr_cursor *only* displays an image on screen. It does not move around
   * when the pointer moves. However, we can attach input devices to it, and
   * it will generate aggregate events for all of them. In these events, we
   * can choose how we want to process them, forwarding them to clients and
   * moving the cursor around. More detail on this process is described in
   * https://drewdevault.com/2018/07/17/Input-handling-in-wlroots.html.
   *
   * And more comments are sprinkled throughout the notify functions above.
   */
  server->cursor_mode = TINYWL_CURSOR_PASSTHROUGH;
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
   */
  wl_list_init(&server->keyboards);
  server->new_input.notify = server_new_input;
  wl_signal_add(&server->backend->events.new_input, &server->new_input);
  server->seat = wlr_seat_create(server->wl_display, "seat0");
  server->request_cursor.notify = seat_request_cursor;
  wl_signal_add(&server->seat->events.request_set_cursor,
                &server->request_cursor);
  server->request_set_selection.notify = seat_request_set_selection;
  wl_signal_add(&server->seat->events.request_set_selection,
                &server->request_set_selection);

  return true;
}

static bool finalize_startup(struct tinywl_server *server,
                             const char *startup_cmd) {
  /* Add a Unix socket to the Wayland display. */
  const char *socket = wl_display_add_socket_auto(server->wl_display);
  if (!socket) {
    wlr_backend_destroy(server->backend);
    return false;
  }

  /* Start the backend. This will enumerate outputs and inputs, become the DRM
   * master, etc */
  if (!wlr_backend_start(server->backend)) {
    wlr_backend_destroy(server->backend);
    wl_display_destroy(server->wl_display);
    return false;
  }

  /* Set the WAYLAND_DISPLAY environment variable to our socket and run the
   * startup command if requested. */
  setenv("WAYLAND_DISPLAY", socket, true);
  if (startup_cmd) {
    if (fork() == 0) {
      execl("/bin/sh", "/bin/sh", "-c", startup_cmd, (void *)NULL);
    }
  }
  wlr_log(WLR_INFO, "Running Wayland compositor on WAYLAND_DISPLAY=%s", socket);
  return true;
}

static bool setup_display_and_backend(struct tinywl_server *server) {
  /* The Wayland display is managed by libwayland. It handles accepting
   * clients from the Unix socket, manging Wayland globals, and so on. */
  server->wl_display = wl_display_create();
  /* The backend is a wlroots feature which abstracts the underlying input and
   * output hardware. The autocreate option will choose the most suitable
   * backend based on the current environment, such as opening an X11 window
   * if an X11 server is running. */
  server->backend = wlr_backend_autocreate(
      wl_display_get_event_loop(server->wl_display), NULL);
  if (server->backend == NULL) {
    wlr_log(WLR_ERROR, "failed to create wlr_backend");
    return false;
  }
  return true;
}

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

  wlr_renderer_init_wl_display(server->renderer, server->wl_display);

  /* Autocreates an allocator for us.
   * The allocator is the bridge between the renderer and the backend. It
   * handles the buffer creation, allowing wlroots to render onto the
   * screen */
  server->allocator = wlr_allocator_autocreate(server->backend, server->renderer);
  if (server->allocator == NULL) {
    wlr_log(WLR_ERROR, "failed to create wlr_allocator");
    return false;
  }

  /* This creates some hands-off wlroots interfaces. The compositor is
   * necessary for clients to allocate surfaces, the subcompositor allows to
   * assign the role of subsurfaces to surfaces and the data device manager
   * handles the clipboard. Each of these wlroots interfaces has room for you
   * to dig your fingers in and play with their behavior if you want. Note
   * that the clients cannot set the selection directly without compositor
   * approval, see the handling of the request_set_selection event below.*/
  struct wlr_compositor *compositor;
  compositor = wlr_compositor_create(server->wl_display, 5, server->renderer);
  wlr_subcompositor_create(server->wl_display);
  wlr_data_device_manager_create(server->wl_display);

  /* Creates an output layout, which a wlroots utility for working with an
   * arrangement of screens in a physical layout. */
  server->output_layout = wlr_output_layout_create(server->wl_display);

  /* Configure a listener to be notified when new outputs are available on the
   * backend. */
  wl_list_init(&server->outputs);
  server->new_output.notify = server_new_output;
  wl_signal_add(&server->backend->events.new_output, &server->new_output);

  /* Create a scene graph. This is a wlroots abstraction that handles all
   * rendering and damage tracking. All the compositor author needs to do
   * is add things that should be rendered to the scene graph at the proper
   * positions and then call wlr_scene_output_commit() to render a frame if
   * necessary.
   */
  server->scene = wlr_scene_create();
  server->scene_layout =
      wlr_scene_attach_output_layout(server->scene, server->output_layout);

  /*
   * We've wired in xwayland to ensure it will start, but we will still need
   * to write client handling
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
 * Return: EXIT_SUCCESS on success, EXIT_FAILURE on failure
 */
int main(int argc, char *argv[]) {
  wlr_log_init(WLR_DEBUG, NULL);
  char *startup_cmd = NULL;

  int args_result = process_args(argc, argv, &startup_cmd);

  if (args_result == -1) {
    exit(EXIT_FAILURE);
  } else if (args_result == 1) {
    exit(EXIT_SUCCESS);
  }

  struct tinywl_server server = {0};

  if (!setup_display_and_backend(&server) || !setup_rendering(&server) ||
      !setup_shell_and_input(&server) ||
      !finalize_startup(&server, startup_cmd)) {
    server_cleanup(&server);
    exit(EXIT_FAILURE);
  }

  /* Run the Wayland event loop. This does not return until you exit the
   * compositor. Starting the backend rigged up all of the necessary event
   * loop configuration to listen to libinput events, DRM events, generate
   * frame events at the refresh rate, and so on. */
  wl_display_run(server.wl_display);

  server_cleanup(&server);

  exit(EXIT_SUCCESS);
}
