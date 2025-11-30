#include "config.h"
#include "utils.h"

const compositor_binding c_bindings[C_BINDINGS_COUNT] = {{XKB_KEY_Escape, terminate_display},
                                          {XKB_KEY_F1, cycle_toplevel},
                                          {XKB_KEY_q, close_focused_surface}};

const user_binding bindings[BINDINGS_COUNT] = {
    {XKB_KEY_Return, "kitty"},
    {XKB_KEY_F, "firefox"},
    {XKB_KEY_e, "kitty ranger"},
    {XKB_KEY_v, "pavucontrol"},
    {XKB_KEY_r, "rofi -show drun"},
    {XKB_KEY_c, "kitty qalc"},
    {XKB_KEY_XF86MonBrightnessUp, "light -A 10"},
    {XKB_KEY_XF86MonBrightnessDown, "light -U 10"},
    {XKB_KEY_XF86AudioPrev, "playerctl previous"},
    {XKB_KEY_XF86AudioNext, "playerctl next"},
    {XKB_KEY_XF86AudioPlay, "playerctl play_pause"},
    {XKB_KEY_XF86AudioRaiseVolume, "pactl set-sink-volume @DEFAULT_SINK@ +10%"},
    {XKB_KEY_XF86AudioLowerVolume, "pactl set-sink-volume @DEFAULT_SINK@ -10%"},
    {XKB_KEY_XF86AudioMute, "pactl set-sink-mute @DEFAULT_SINK@ toggle"}};

const compositor_binding *get_c_bindings(void) {
    return c_bindings;
}

const user_binding *get_bindings(void) {
    return bindings;
}
