#include "toplevel.h"
#include "utils.h"
#include "input.h"
#include <stdlib.h>


static void xdg_toplevel_map(struct wl_listener *listener, void *data) {
  (void)data; // data is unused here
  /* Called when the surface is mapped, or ready to display on-screen. */
  struct tinywl_toplevel *toplevel = wl_container_of(listener, toplevel, map);

  wl_list_insert(&toplevel->server->toplevels, &toplevel->link);

  focus_toplevel(toplevel);
}

static void xdg_toplevel_unmap(struct wl_listener *listener, void *data) {
  (void)data; // data is unused here
  /* Called when the surface is unmapped, and should no longer be shown. */
  struct tinywl_toplevel *toplevel = wl_container_of(listener, toplevel, unmap);

  /* Reset the cursor mode if the grabbed toplevel was unmapped. */
  if (toplevel == toplevel->server->grabbed_toplevel) {
    reset_cursor_mode(toplevel->server);
  }

  wl_list_remove(&toplevel->link);
}

static void xdg_toplevel_commit(struct wl_listener *listener, void *data) {
  (void)data; // data is unused here
  /* Called when a new surface state is committed. */
  struct tinywl_toplevel *toplevel =
      wl_container_of(listener, toplevel, commit);

  if (toplevel->xdg_toplevel->base->initial_commit) {
    /* When an xdg_surface performs an initial commit, the compositor must
     * reply with a configure so the client can map the surface. tinywl
     * configures the xdg_toplevel with 0,0 size to let the client pick the
     * dimensions itself. */
    wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, 0, 0);
  }

  // Update border dimensions based on surface size
  struct wlr_box *geo_box = &toplevel->xdg_toplevel->base->geometry;
  int border_width = 2;

  wlr_scene_rect_set_size(toplevel->border_top, geo_box->width, border_width);
  wlr_scene_rect_set_size(toplevel->border_bottom, geo_box->width,
                          border_width);
  wlr_scene_rect_set_size(toplevel->border_left, border_width, geo_box->height);
  wlr_scene_rect_set_size(toplevel->border_right, border_width,
                          geo_box->height);

  wlr_scene_node_set_position(&toplevel->border_top->node, geo_box->x,
                              geo_box->y - border_width);
  wlr_scene_node_set_position(&toplevel->border_bottom->node, geo_box->x,
                              geo_box->y + geo_box->height);
  wlr_scene_node_set_position(&toplevel->border_left->node,
                              geo_box->x - border_width, geo_box->y);
  wlr_scene_node_set_position(&toplevel->border_right->node,
                              geo_box->x + geo_box->width, geo_box->y);
}

static void xdg_toplevel_destroy(struct wl_listener *listener, void *data) {
  (void)data; // unused here
  /* Called when the xdg_toplevel is destroyed. */
  struct tinywl_toplevel *toplevel =
      wl_container_of(listener, toplevel, destroy);

  wl_list_remove(&toplevel->map.link);
  wl_list_remove(&toplevel->unmap.link);
  wl_list_remove(&toplevel->commit.link);
  wl_list_remove(&toplevel->destroy.link);
  wl_list_remove(&toplevel->request_move.link);
  wl_list_remove(&toplevel->request_resize.link);
  wl_list_remove(&toplevel->request_maximize.link);
  wl_list_remove(&toplevel->request_fullscreen.link);

  free(toplevel);
}

static void xdg_toplevel_request_move(struct wl_listener *listener,
                                      void *data) {
  (void)data; // data is unused here
  /* This event is raised when a client would like to begin an interactive
   * move, typically because the user clicked on their client-side
   * decorations. Note that a more sophisticated compositor should check the
   * provided serial against a list of button press serials sent to this
   * client, to prevent the client from requesting this whenever they want. */
  struct tinywl_toplevel *toplevel =
      wl_container_of(listener, toplevel, request_move);
  begin_interactive(toplevel, TINYWL_CURSOR_MOVE, 0);
}

static void xdg_toplevel_request_resize(struct wl_listener *listener,
                                        void *data) {
  (void)data; // data is unused here
  /* This event is raised when a client would like to begin an interactive
   * resize, typically because the user clicked on their client-side
   * decorations. Note that a more sophisticated compositor should check the
   * provided serial against a list of button press serials sent to this
   * client, to prevent the client from requesting this whenever they want. */
  struct wlr_xdg_toplevel_resize_event *event = data;
  struct tinywl_toplevel *toplevel =
      wl_container_of(listener, toplevel, request_resize);
  begin_interactive(toplevel, TINYWL_CURSOR_RESIZE, event->edges);
}

static void xdg_toplevel_request_maximize(struct wl_listener *listener,
                                          void *data) {
  (void)data; // data is unused here
  /* This event is raised when a client would like to maximize itself,
   * typically because the user clicked on the maximize button on client-side
   * decorations. tinywl doesn't support maximization, but to conform to
   * xdg-shell protocol we still must send a configure.
   * wlr_xdg_surface_schedule_configure() is used to send an empty reply.
   * However, if the request was sent before an initial commit, we don't do
   * anything and let the client finish the initial surface setup. */
  struct tinywl_toplevel *toplevel =
      wl_container_of(listener, toplevel, request_maximize);
  if (toplevel->xdg_toplevel->base->initialized) {
    wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
  }
}

static void xdg_toplevel_request_fullscreen(struct wl_listener *listener,
                                            void *data) {
  (void)data; // data is unused here
  /* Just as with request_maximize, we must send a configure here. */
  struct tinywl_toplevel *toplevel =
      wl_container_of(listener, toplevel, request_fullscreen);
  if (toplevel->xdg_toplevel->base->initialized) {
    wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
  }
}


void server_new_xdg_toplevel(struct wl_listener *listener, void *data) {
  /* This event is raised when a client creates a new toplevel (application
   * window). */
  struct tinywl_server *server =
      wl_container_of(listener, server, new_xdg_toplevel);
  struct wlr_xdg_toplevel *xdg_toplevel = data;

  /* Allocate a tinywl_toplevel for this surface */
  struct tinywl_toplevel *toplevel = calloc(1, sizeof(*toplevel));
  toplevel->server = server;
  toplevel->xdg_toplevel = xdg_toplevel;
  toplevel->scene_tree = wlr_scene_xdg_surface_create(
      &toplevel->server->scene->tree, xdg_toplevel->base);
  toplevel->scene_tree->node.data = toplevel;
  xdg_toplevel->base->data = toplevel->scene_tree;

  // Create borders
  int border_width = 2;
  float border_color[4] = {1.0f, 0.647f, 0.0f, 1.0f};

  /* This is currently borked for lutris */
  toplevel->border_top = wlr_scene_rect_create(toplevel->scene_tree, 0,
                                               border_width, border_color);
  toplevel->border_bottom = wlr_scene_rect_create(toplevel->scene_tree, 0,
                                                  border_width, border_color);
  toplevel->border_left = wlr_scene_rect_create(toplevel->scene_tree,
                                                border_width, 0, border_color);
  toplevel->border_right = wlr_scene_rect_create(toplevel->scene_tree,
                                                 border_width, 0, border_color);

  // Position borders (they'll be updated on commit)
  wlr_scene_node_set_position(&toplevel->border_top->node, 0, -border_width);
  wlr_scene_node_set_position(&toplevel->border_left->node, -border_width, 0);

  /* Listen to the various events it can emit */
  toplevel->map.notify = xdg_toplevel_map;
  wl_signal_add(&xdg_toplevel->base->surface->events.map, &toplevel->map);
  toplevel->unmap.notify = xdg_toplevel_unmap;
  wl_signal_add(&xdg_toplevel->base->surface->events.unmap, &toplevel->unmap);
  toplevel->commit.notify = xdg_toplevel_commit;
  wl_signal_add(&xdg_toplevel->base->surface->events.commit, &toplevel->commit);

  toplevel->destroy.notify = xdg_toplevel_destroy;
  wl_signal_add(&xdg_toplevel->events.destroy, &toplevel->destroy);

  /* cotd */
  toplevel->request_move.notify = xdg_toplevel_request_move;
  wl_signal_add(&xdg_toplevel->events.request_move, &toplevel->request_move);
  toplevel->request_resize.notify = xdg_toplevel_request_resize;
  wl_signal_add(&xdg_toplevel->events.request_resize,
                &toplevel->request_resize);
  toplevel->request_maximize.notify = xdg_toplevel_request_maximize;
  wl_signal_add(&xdg_toplevel->events.request_maximize,
                &toplevel->request_maximize);
  toplevel->request_fullscreen.notify = xdg_toplevel_request_fullscreen;
  wl_signal_add(&xdg_toplevel->events.request_fullscreen,
                &toplevel->request_fullscreen);
}
