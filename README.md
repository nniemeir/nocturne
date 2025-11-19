# Nocturne

Wayland compositor using TinyWL as a starting point. This is being developed for learning purposes and is not intended for production use. 

## Future Plans
* Wallpaper Support
* Xwayland
* Dynamic tiling (Like Sway)
* Floating toggle (Like Sway)
* Builtin top bar (Like my existing waybar config, dead simple)
* Modularization of project 
* Configuration being simplified but still set at compile-time

## Dependencies
* GCC
* GNU make
* wlroots development library
* wayland-protocols development library

## Installation
Compile the project
```
make
```

### Make Targets 
- `make` - Compile the binary
- `make clean` – Remove build objects
- `make fclean` - Remove build objects and binary

## Usage
```
nocturne [OPTIONS]
```

### Options
```
-s <command>         Specify command to run on startup
-h                   Display program usage
```

## Keybindings (Currently Hardcoded)
* 'Win+Escape': Terminate the compositor
* 'Win+F1': Cycle between windows
* 'Win+Return': Open Kitty Terminal
* 'Win+e': Open Ranger file manager in kitty
* 'Win+E': Open Thunar file manager
* 'Win+F': Open Firefox

## License
GNU General Public License V2

Copyright (c) 2025 Jacob Niemeir
