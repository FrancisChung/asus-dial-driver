# asus-dial-driver

A Linux driver and companion GUI for the Asus Dial hardware. Currently tested on Linux Mint (Ubuntu).

This is based on a fork of https://github.com/fredaime/openwheel/ with fixes to get the daemon
working, plus a new Qt6/QML tray + on-screen overlay (`openwheel-gadget`) that turns the dial into
a Surface-Dial-style control for volume, screen brightness, scrolling, and media playback.

## What's here

- **`openwheel-daemon`** — reads the Asus Dial's raw HID input and emits `Rotate`/`Press` events as
  D-Bus signals on the session bus (`org.asus.dial` / `/org/asus/dial`).
- **`openwheel-gadget`** — a Qt6/QML tray icon and on-screen overlay that listens to those D-Bus
  signals and turns them into an actual usable dial: a radial menu (Surface-Dial-style) for picking
  which function the dial controls, live on-screen feedback, and a system tray with an
  enable/disable toggle and quit.

## Features

- **System tray icon** — reflects whether the daemon is currently connected (i.e. whether
  `org.asus.dial` is owned on the session bus); its menu has an Enabled toggle (pause the dial's
  effect without stopping the daemon) and Quit.
- **Radial menu** — press and hold the dial to open a menu of 4 functions, rotate while still
  holding to move the highlighted selection, release to confirm. While open, the overlay shows an
  "ASUS Dial" branded logo in the center; functions that aren't currently available (e.g. Media
  with no player running) are shown dimmed.
- **Four dial functions:**
  - **Volume** — adjusts system volume via `pactl` (PulseAudio/PipeWire).
  - **Brightness** — adjusts screen brightness via `systemd-logind`, always reading live hardware
    state (so it won't drift out of sync if brightness changes some other way while the gadget is
    running — e.g. a hotkey or another app).
  - **Scroll** — synthesizes scroll-wheel input into whichever window currently has focus (X11 via
    the XTest extension by default; Wayland via a `uinput` virtual device, needs one-time setup —
    see below).
  - **Media** — seeks ±5 seconds in whatever's currently playing, via MPRIS (works with most Linux
    media players).
- **On-screen feedback** — every rotate briefly shows a HUD with the function's name and current
  value (e.g. "Volume: 62%"), so a glance always tells you what the dial is controlling, even after
  not touching it for a while. Confirming a menu selection shows the same HUD with the
  newly-picked function's name (e.g. "Scroll").
- **Remembers your last pick** — the active function persists across restarts, defaulting to
  Volume on first run.

## How to use it

1. **Quick rotate** (dial not held down) — adjusts whichever function is currently active. A HUD
   pops up briefly (e.g. "Volume: 62%") and fades out after about 1.5 seconds.
2. **Press and hold** the dial — after about 400ms, a radial menu pops up in the middle of your
   screen showing all 4 functions (Volume / Scroll / Brightness / Media) arranged in a circle, with
   the "ASUS Dial" logo in the center.
3. **While still holding**, rotate to move the highlight between the 4 options.
4. **Release** the dial — this confirms whichever option is highlighted as the new active
   function, closes the menu, and briefly shows a HUD confirming what you picked. This is one
   continuous gesture (hold → rotate → release) — no second press needed.
5. **Tray icon** — open its context menu for the Enabled toggle and Quit. The icon itself changes
   if the daemon disconnects (e.g. if `asus_wheel` isn't running).

## Building and launching

### Quick start (recommended)

From the repo root, after installing the dependencies below:
```bash
./launch-openwheel.sh
```
This builds the daemon and gadget if they aren't already built, starts `openwheel-daemon` if
nothing already owns `org.asus.dial` on the session bus (reusing an already-running daemon
otherwise), waits for it to register, then launches `openwheel-gadget`. It only stops the daemon it
started itself when you quit the gadget — it won't touch a daemon started some other way.

### Manual build

**Daemon:**
```bash
cd openwheel-daemon
cmake .
make
```
Produces `openwheel-daemon/asus_wheel`. It needs read access to the dial's HID device, which it
finds automatically at startup by scanning `/sys/class/hidraw` for the ASUS2020 device (the
hidraw number shifts depending on what else is plugged in, e.g. docks/hubs). Run it with
`./asus_wheel`.

**Gadget:**
```bash
cd openwheel-gadget
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```
Produces `openwheel-gadget/build/openwheel-gadget`. Run it with `./openwheel-gadget` — it needs the
daemon (or anything else emitting the same D-Bus signals) running to do anything. For testing
without physical hardware, you can inject signals directly:
```bash
dbus-send --session --type=signal /org/asus/dial org.asus.dial.Rotate int32:1
dbus-send --session --type=signal /org/asus/dial org.asus.dial.Press int32:1
```
(`Rotate` takes `int32` ±1 per detent; `Press` takes `int32` 1 on press / 0 on release — both on
`org.asus.dial` / `/org/asus/dial`, session bus.)

### Dependencies

Build (Debian/Ubuntu naming): `qt6-base-dev qt6-declarative-dev libqt6svg6-dev libxtst-dev`.

Runtime dependencies (also needed to actually run the built binary, not just compile it):
`qml6-module-qtquick qml6-module-qtquick-window qml6-module-qtqml-workerscript`. Without these,
the binary builds and links fine but exits immediately with "module ... is not installed" QML
errors as soon as it tries to load `DialOverlay.qml`.

### Autostart

To start automatically with your session, first symlink the built binary to your PATH (run from inside
the `build/` directory used above):
```bash
mkdir -p ~/.local/bin
ln -sf "$(pwd)/openwheel-gadget" ~/.local/bin/openwheel-gadget
```

Then copy `openwheel-gadget/openwheel-gadget.desktop` to `~/.config/autostart/`. This handles the
gadget only — for the daemon to also start automatically, set up your own systemd user service or
equivalent (not provided here).

### Wayland scroll support (optional)

The Scroll dial function uses X11's XTest extension by default and works out of the box on any X11
session. On Wayland, scroll instead uses a `uinput` virtual device, which requires one-time setup:
add your user to the `input` group (`sudo usermod -aG input $USER`, then log out and back in) so
the gadget can open `/dev/uinput` without root. If this isn't set up, the Scroll entry in the radial
menu is simply disabled — every other function works normally on Wayland regardless.

### Running tests

```bash
QT_QPA_PLATFORM=offscreen ctest --output-on-failure
```
(run from `openwheel-gadget/build/`, no display server needed). If any test involving D-Bus fails,
or if you have `openwheel-daemon` (or any other service already registered on `org.asus.dial`)
running while testing, wrap the command in `dbus-run-session --`, e.g. `dbus-run-session -- env
QT_QPA_PLATFORM=offscreen ctest --output-on-failure` — some tests take temporary ownership of that
D-Bus name, which conflicts with a real daemon (or any other owner) already holding it on your
regular session bus.

## Known limitations (v1)

- Function icons aren't rendered — the radial menu and HUD are text-only. `DialFunction::iconName()`
  is fully wired through but unused; rendering real icons later just needs a `QQuickImageProvider`,
  no changes to any already-built code.
- No per-app context — the active function is global, not tied to whichever app currently has
  focus.
- No rotation-speed sensitivity — the daemon reports rotation direction only, not how fast you're
  turning the dial.
- Only tested on Linux Mint (Ubuntu).
