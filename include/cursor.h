#ifndef CURSOR_H
#define CURSOR_H

#include "server.h"

/* 
 * tinywl_cursor_mode enum is defined in server.h because tinywl_server struct
 * depends on it.
*/

void reset_cursor_mode(struct tinywl_server *server);
void process_cursor_move(struct tinywl_server *server);
void process_cursor_resize(struct tinywl_server *server);
void process_cursor_motion(struct tinywl_server *server, uint32_t time);
void server_cursor_button(struct wl_listener *listener, void *data);
void server_cursor_motion(struct wl_listener *listener, void *data);
void server_cursor_motion_absolute(struct wl_listener *listener, void *data);
void server_cursor_axis(struct wl_listener *listener, void *data);
void server_cursor_frame(struct wl_listener *listener, void *data);
void begin_interactive(struct tinywl_toplevel *toplevel,
                       enum tinywl_cursor_mode mode, uint32_t edges);

#endif
