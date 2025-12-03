/**
 * popup.h
 *
 * Popup surface management.
 *
 * OVERVIEW:
 * Popups are temporary surfaces that appear above regular windows. They're used
 * for UI elements like:
 * - Context menus
 * - Dropdown menus in menu bars
 * - Tooltips
 * - Autocomplete suggestions
 * - Date pickers
 * - Combo box dropdowns
 *
 * POPUP vs TOPLEVEL:
 * - Toplevel: A main application window
 * - Popup: A child surface attached to a toplevel or another popup
 *
 * XDG_POPUP PROTOCOL:
 * The xdg_popup protocol (part of xdg_shell) defines how popups work:
 * - Parent: Must be attached to a parent surface (toplevel or popup)
 * - Positioner: Rules for where to place the popup
 * - Grab: Can grab input to be modal
 * - Dismiss: Compositor can dismiss popups
 *
 * POPUP POSITIONING:
 * Clients provide a "positioner" with rules like:
 * - Anchor: Point on parent to attach to
 * - Gravity: Which direction popup grows from anchor
 * - Size: Desired popup size
 * - Constraints: What to do if popup would go off-screen
 *   * Slide: Move along an axis to stay on-screen
 *   * Flip: Switch to opposite side of anchor
 *   * Resize: Shrink popup to fit
 */

#ifndef POPUP_H
#define POPUP_H

#include <wayland-server-core.h>

/**
 * struct tinywl_popup - Represents a popup surface
 * @xdg_popup: The underlying wlroots xdg_popup object
 * @commit: Listener for surface commit events
 * @destroy: Listener for popup destruction
 *
 * Popups are simpler than toplevels - they can't be moved or resized
 * independently. The struct tracks:
 * - The xdg_popup object
 * - Event listeners for lifecycle
 *
 * COMMIT EVENT:
 * Called when the client commits new state to the popup surface. The initial
 * commit is special, we must send a configure event so that the client can map
 * the surface.
 *
 * DESTROY EVENT:
 * Called when the popup is destroyed. We clean up the tinywl_popup struct and remove listeners.
 * 
 * SCENE GRAPH:
 * Popups are added to the scene graph as children of their parent's scene tree. This ensures:
 * - Popup renders above parent
 * - Popup moves with parent
 * - Popup is clipped to output boundaries
 * - Popup is properly destroyed when parent is destroyed
 */
struct tinywl_popup {
  /* The underlying wlroots xdg_popup object */
  struct wlr_xdg_popup *xdg_popup;
  
  /* Event listeners */
  struct wl_listener commit; /* Surface state was committed */
  struct wl_listener destroy; /* Popup was destroyed */
};

/**
 * server_new_xdg_popup - Handles new popup surface creation
 * @listener: Wayland listener that triggered this callback
 * @data: Pointer to wlr_xdg_popup
 *
 * Called when a client creates a new popup surface. This function:
 *
 * 1. Allocates tinywl_popup struct to track the popup
 *
 * 2. Finds the parent surface
 *    - Popups must have a parent (toplevel or another popup)
 *    - Parent is specified by the client in the xdg_popup creation
 *
 * 3. Adds popup to scene graph
 *    - Uses wlr_scene_xdg_surface_create() helper
 *    - Creates scene node as child of parent's scene tree
 *    - This makes popup render above parent and move with it
 *
 * 4. Stores scene tree in xdg_popup->base->data
 *    - This allows finding the scene node from the xdg_surface
 *    - Used for nested popups to find their parent scene node
 *
 * 5. Registers event listeners
 *    - commit: Handle surface state commits
 *    - destroy: Clean up when popup is destroyed
 *
 * PARENT LOOKUP:
 * We use wlr_xdg_surface_try_from_wlr_surface() to find the parent
 * xdg_surface, then access its ->data field to get the parent's scene
 * tree. This relies on us setting the data field when creating toplevels
 * and popups.
 *
 * INITIAL COMMIT:
 * The first commit is special. The client creates the popup, commits
 * initial state, then waits for a configure event from us before
 * actually showing anything. We send an empty configure on the initial
 * commit to allow the popup to map.
 *
 * POPUP DISMISSAL:
 * The compositor can dismiss popups by sending a popup_done event.
 * Common reasons to dismiss:
 * - User clicked outside the popup
 * - User pressed Escape
 * - Parent window was unmapped
 * - Focus changed to a different toplevel
 *
 * We don't currently implement popup dismissal logic, but it would
 * go here or in the cursor button handler.
 */
void server_new_xdg_popup(struct wl_listener *listener, void *data);

#endif
