#ifndef CONFIG_H
#define CONFIG_H

#include <xkbcommon/xkbcommon.h>

#include "server.h"

#define C_BINDINGS_COUNT 3
#define BINDINGS_COUNT 14

/* User would set this to any of the modifier keys per their preference */
#define MODKEY WLR_MODIFIER_ALT

typedef struct {
  xkb_keysym_t key;
  void (*fptr)(struct tinywl_server *server);
} compositor_binding;

typedef struct {
  xkb_keysym_t key;
  char *command;
} user_binding;

const compositor_binding *get_c_bindings(void);
const user_binding *get_bindings(void);

#endif
