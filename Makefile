
PKG_CONFIG ?= pkg-config

WAYLAND_PROTOCOLS := $(shell $(PKG_CONFIG) --variable=pkgdatadir wayland-protocols)
WAYLAND_SCANNER   := $(shell $(PKG_CONFIG) --variable=wayland_scanner wayland-scanner)

PKGS = wlroots-0.19 wayland-server xkbcommon

CFLAGS_PKG_CONFIG := $(shell $(PKG_CONFIG) --cflags $(PKGS))
CFLAGS += $(CFLAGS_PKG_CONFIG) -Wall -Wextra -pedantic -g -I include -DWLR_USE_UNSTABLE

LIBS := $(shell $(PKG_CONFIG) --libs $(PKGS))

SRC := $(wildcard src/*.c)

NAME = nocturne

CC = gcc

BIN_DIR = bin

BUILD_DIR = build

OBJS = $(SRC:src/%.c=$(BUILD_DIR)/%.o)

all: bin $(BIN_DIR)/$(NAME)

$(BIN_DIR)/$(NAME): $(OBJS)
	$(CC) -o $@ $^ $(LIBS)

$(BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) -g $(CFLAGS) -c $< -o $@

bin:
	mkdir -p $(BIN_DIR)

# wayland-scanner is a tool which generates C headers and rigging for Wayland
# protocols, which are specified in XML. wlroots requires you to rig these up
# to your build system yourself and provide them in the include path.
xdg-shell-protocol.h:
	$(WAYLAND_SCANNER) server-header \
		$(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml $@

tinywl.o: tinywl.c xdg-shell-protocol.h
	$(CC) -c $< -g -Werror $(CFLAGS) -o $@

tinywl: tinywl.o
	$(CC) $^ -g -Werror $(CFLAGS) $(LDFLAGS) $(LIBS) -o $@

clean:
	rm -f xdg-shell-protocol.h
	rm -f $(OBJS)
	rm -rf $(BUILD_DIR)

fclean: clean
	rm -rf $(BIN_DIR)

.PHONY: all bin clean fclean
