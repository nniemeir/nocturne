#include "server.h"

void server_cleanup(struct tinywl_server *server) {
  wl_display_destroy_clients(server->wl_display);

  wl_list_remove(&server->new_xdg_toplevel.link);
  wl_list_remove(&server->new_xdg_popup.link);

  wl_list_remove(&server->cursor_motion.link);
  wl_list_remove(&server->cursor_motion_absolute.link);
  wl_list_remove(&server->cursor_button.link);
  wl_list_remove(&server->cursor_axis.link);
  wl_list_remove(&server->cursor_frame.link);

  wl_list_remove(&server->new_input.link);
  wl_list_remove(&server->request_cursor.link);
  wl_list_remove(&server->request_set_selection.link);

  wl_list_remove(&server->new_output.link);

  wlr_scene_node_destroy(&server->scene->tree.node);
  wlr_xcursor_manager_destroy(server->cursor_mgr);
  wlr_cursor_destroy(server->cursor);
  wlr_allocator_destroy(server->allocator);
  wlr_renderer_destroy(server->renderer);
  wlr_backend_destroy(server->backend);
  wl_display_destroy(server->wl_display);
}
