# GLTeapot

A macOS/Linux replica of [Haiku](https://www.haiku-os.org/)'s (originally
BeOS's) `GLTeapot` sample application: one or more spinning, draggable Utah
teapots rendered with fixed-function OpenGL, controlled from an in-window
menu bar just like the original BeOS app (the menu bar is a widget inside
the window, not a system menu bar).

This is a fresh implementation, not a port of Haiku's C++/BeAPI sources —
it targets GLFW + OpenGL + Dear ImGui instead of BDirectWindow/BGLView, but
reproduces the original's behavior, defaults, and menu structure closely
(see "Differences from the original" below).

## Building

Requires CMake 3.16+, a C++17 compiler, and OpenGL development headers.
GLFW and Dear ImGui are fetched automatically via CMake `FetchContent` if
not already installed system-wide (requires network access on first
configure).

### macOS

```sh
xcode-select --install   # if you don't already have the command line tools
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/GLTeapot
```

### Linux

Install the usual OpenGL/X11 development packages GLFW needs, e.g. on
Debian/Ubuntu:

```sh
sudo apt install build-essential cmake libgl1-mesa-dev \
    libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

(Or, to skip building GLFW from source, `sudo apt install libglfw3-dev` —
CMake will use it automatically if `find_package(glfw3)` succeeds.)

When CMake fetches and builds GLFW itself, it only builds GLFW's X11
backend (Wayland is off by default here, since building it requires
`wayland-scanner`/`wayland-protocols`/`libxkbcommon-dev` for no benefit on
an X11-only machine). This also works fine under XWayland. If you do want
a native Wayland backend, install those packages and reconfigure with
`-DGLFW_BUILD_WAYLAND=ON`.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/GLTeapot
```

## Controls

| Action | Effect |
|---|---|
| Left-click + drag on a teapot | Rotate it |
| Release while still moving | Keeps spinning with that momentum |
| Right-click + drag on a teapot | Move it (hold Shift to move in/out instead of up/down) |
| Middle-click on a teapot | Open its color / opacity (Solid, Translucent, Transparent) menu |
| `Ctrl+N` / `Cmd+N`, or GLTeapot ▸ Add a teapot | Add another teapot |
| `Ctrl+Q` / `Cmd+Q`, or GLTeapot ▸ Quit | Quit |

The **Settings** menu toggles perspective vs. orthographic projection, the
FPS overlay, filled vs. wireframe polygons, lighting, backface culling,
depth testing, Gouraud vs. flat shading, fog, and vsync. The **Lights**
menu independently sets the color (or turns off) each of the scene's three
lights (Upper center, Lower left, Right) — the same structure and defaults
as the original.

## Differences from the original

- The original's per-vertex teapot mesh was baked into the app as resource
  data exported from a 3D modeler; this version tessellates the classic
  10-patch Utah teapot Bezier data at startup using the OpenGL evaluator
  API (`glMap2d`/`glEvalMesh2`), the same technique GLUT's
  `glutSolidTeapot()` uses. Visually equivalent, but not vertex-identical.
- Object picking (for drag/rotate/context-menu targeting) uses the same
  "draw flat-shaded ID colors into the back buffer, read a pixel, redraw
  normally" trick as the original's `ObjectView::ObjectAtPoint()`.
- No direct-frame-buffer/BDirectWindow-specific behavior, since that's a
  BeOS/Haiku-only API; GLFW's normal double-buffered GL context is used
  instead.

## Attribution

- Application design/behavior/menu structure: derived from Haiku's
  `src/apps/glteapot`, itself derived from a Be, Inc. sample app
  (`GLTeapot`), licensed under the Be Sample Code License; later Haiku-era
  changes (e.g. the quaternion-based rotation this replica also uses) are
  under the MIT License. See <https://github.com/haiku/haiku/tree/master/src/apps/glteapot>.
- Utah teapot control-point/patch data (`src/TeapotGeometry.cpp`):
  the classic 10-patch data set originally released by Silicon Graphics,
  Inc., as long distributed with GLUT/freeglut's `glutSolidTeapot()`. See
  the notice at the top of that file.
- This repository's own source code is otherwise available under the MIT
  License (see `LICENSE`).
