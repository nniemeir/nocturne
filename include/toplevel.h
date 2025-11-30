#ifndef TOPLEVEL_H
#define TOPLEVEL_H

#include <wayland-server-core.h>

#include "server.h"

/**
 * struct tinywl_toplevel - Represents a top-level window (application window)
 *
 * In Wayland, clients provide surfaces, and XDG Shell turns them into toplevels
 * (main windows). This struct manages one of these windows, including its
 * position in the scene, borders, and event listeners for mapping, resizing,
 * etc.
 */
struct tinywl_toplevel {
  struct wl_list link;
  struct tinywl_server *server;
  struct wlr_xdg_toplevel *xdg_toplevel;
  struct wlr_scene_tree *scene_tree;

  struct wlr_scene_rect *border_top;
  struct wlr_scene_rect *border_bottom;
  struct wlr_scene_rect *border_left;
  struct wlr_scene_rect *border_right;

  struct wl_listener map;
  struct wl_listener unmap;
  struct wl_listener commit;
  struct wl_listener destroy;
  struct wl_listener request_move;
  struct wl_listener request_resize;
  struct wl_listener request_maximize;
  struct wl_listener request_fullscreen;
};

void server_new_xdg_toplevel(struct wl_listener *listener, void *data);

#endif
