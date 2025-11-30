#ifndef OUTPUT_H
#define OUTPUT_H

#include <wayland-server-core.h>

#include "server.h"

/**
 * struct tinywl_output - Represents a single display/output
 *
 * Outputs are where the compositor renders the final image. This struct tracks
 * the output's state and listeners for events like frame rendering or
 * destruction.
 */
struct tinywl_output {
  struct wl_list link;
  struct tinywl_server *server;
  struct wlr_output *wlr_output;
  struct wl_listener frame;
  struct wl_listener request_state;
  struct wl_listener destroy;
};

void server_new_output(struct wl_listener *listener, void *data);

#endif
