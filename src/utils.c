#include <signal.h>
#include <unistd.h>

#include "toplevel.h"
#include "utils.h"

/*
 * execute_program - Forks and executes a shell command
 *
 * Used to launch external programs in response to keybindings.
 */
void execute_program(char *name) {
  if (fork() == 0) {
    execl("/bin/sh", "/bin/sh", "-c", name, (void *)NULL);
  }
}

/**
 * focus_toplevel - Sets keyboard focus to a specific window
 * @toplevel - The toplevel to focus
 *
 * Keyboard focus means that the window receives key events. This function
 * deactivates the previous focused window, moves the new one to the front, and
 * notifies the seat (input manager) to route input there.
 *
 * Pointer (mouse) focus is handled separately.
 */
void focus_toplevel(struct tinywl_toplevel *toplevel) {
  /* Note: this function only deals with keyboard focus. */
  if (toplevel == NULL) {
    return;
  }
  struct tinywl_server *server = toplevel->server;
  struct wlr_seat *seat = server->seat;
  struct wlr_surface *prev_surface = seat->keyboard_state.focused_surface;
  struct wlr_surface *surface = toplevel->xdg_toplevel->base->surface;
  if (prev_surface == surface) {
    /* Don't re-focus an already focused surface. */
    return;
  }
  if (prev_surface) {
    /*
     * Deactivate the previously focused surface. This lets the client know
     * it no longer has focus and the client will repaint accordingly, e.g.
     * stop displaying a caret.
     */
    struct wlr_xdg_toplevel *prev_toplevel =
        wlr_xdg_toplevel_try_from_wlr_surface(prev_surface);
    if (prev_toplevel != NULL) {
      wlr_xdg_toplevel_set_activated(prev_toplevel, false);
    }
  }

  struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(seat);
  /* Move the toplevel to the front */
  wlr_scene_node_raise_to_top(&toplevel->scene_tree->node);
  wl_list_remove(&toplevel->link);
  wl_list_insert(&server->toplevels, &toplevel->link);
  /* Activate the new surface */
  wlr_xdg_toplevel_set_activated(toplevel->xdg_toplevel, true);
  /*
   * Tell the seat to have the keyboard enter this surface. wlroots will keep
   * track of this and automatically send key events to the appropriate
   * clients without additional work on your part.
   */
  if (keyboard != NULL) {
    wlr_seat_keyboard_notify_enter(seat, surface, keyboard->keycodes,
                                   keyboard->num_keycodes,
                                   &keyboard->modifiers);
  }
}

struct tinywl_toplevel *desktop_toplevel_at(struct tinywl_server *server,
                                                   double lx, double ly,
                                                   struct wlr_surface **surface,
                                                   double *sx, double *sy) {
  /* This returns the topmost node in the scene at the given layout coords.
   * We only care about surface nodes as we are specifically looking for a
   * surface in the surface tree of a tinywl_toplevel. */
  struct wlr_scene_node *node =
      wlr_scene_node_at(&server->scene->tree.node, lx, ly, sx, sy);
  if (node == NULL || node->type != WLR_SCENE_NODE_BUFFER) {
    return NULL;
  }
  struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_from_node(node);
  struct wlr_scene_surface *scene_surface =
      wlr_scene_surface_try_from_buffer(scene_buffer);
  if (!scene_surface) {
    return NULL;
  }

  *surface = scene_surface->surface;
  /* Find the node corresponding to the tinywl_toplevel at the root of this
   * surface tree, it is the only one for which we set the data field. */
  struct wlr_scene_tree *tree = node->parent;
  while (tree != NULL && tree->node.data == NULL) {
    tree = tree->node.parent;
  }
  return tree->node.data;
}

void close_focused_surface(struct tinywl_server *server) {
  struct wlr_surface *focused_surface =
      server->seat->keyboard_state.focused_surface;
  // STUDY THIS BIT MORE, IMPORTANT
  // WE FIND WHAT HAS KEYBOARD FOCUS AND SEND KILL SIGNAL TO IT.
  if (focused_surface) {
    struct wl_client *client =
        wl_resource_get_client(focused_surface->resource);
    pid_t pid;
    wl_client_get_credentials(client, &pid, NULL, NULL);
    wlr_log(WLR_INFO, "Keyboard-focused window PID: %d", pid);
    kill(pid, SIGTERM);
  }
}

void cycle_toplevel(struct tinywl_server *server) {
  if (wl_list_length(&server->toplevels) < 2) {
    return;
  }
  struct tinywl_toplevel *next_toplevel =
      wl_container_of(server->toplevels.prev, next_toplevel, link);
  focus_toplevel(next_toplevel);
}

void terminate_display(struct tinywl_server *server) {
  wl_display_terminate(server->wl_display);
}
