#ifndef POPUP_H
#define POPUP_H

#include <wayland-server-core.h>

/**
 * struct tinywl_popup - Represents a popup surface (e.g., menus, tooltips)
 *
 * Popups are temporary surfaces attached to toplevels, this handles their
 * lifecycle.
 */
struct tinywl_popup {
  struct wlr_xdg_popup *xdg_popup;
  struct wl_listener commit;
  struct wl_listener destroy;
};

void server_new_xdg_popup(struct wl_listener *listener, void *data);

#endif
