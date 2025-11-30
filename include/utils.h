#ifndef UTILS_H
#define UTILS_H

#include "server.h"

void execute_program(char *name);
void focus_toplevel(struct tinywl_toplevel *toplevel);
struct tinywl_toplevel *desktop_toplevel_at(struct tinywl_server *server,
                                            double lx, double ly,
                                            struct wlr_surface **surface,
                                            double *sx, double *sy);
void close_focused_surface(struct tinywl_server *server);
void cycle_toplevel(struct tinywl_server *server);
void terminate_display(struct tinywl_server *server);

#endif
