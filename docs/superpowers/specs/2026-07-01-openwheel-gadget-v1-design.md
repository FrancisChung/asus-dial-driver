# openwheel-gadget v1 Design

## Context

`openwheel-daemon` reads the Asus Dial's HID reports and emits two D-Bus signals
on the session bus (`org.asus.dial`, object `/org/asus/dial`):

- `Rotate` (int32: `1` or `-1` per detent)
- `Press` (int32: `1` on press, `0` on release)

`openwheel-gadget` is currently an empty stub (`main.cpp` is a bare comment,
`DialOverlay.qml` is empty, `CMakeLists.txt` declares no executable or Qt
linkage). This spec defines v1: a GUI companion that turns those signals into
a usable on-screen experience, inspired by Microsoft's Surface Dial ("Wheel")
software and ASUS ProArt Creator Hub.

v1 scope is intentionally limited to what the daemon already emits today — no
daemon changes are required. Speed-sensitive rotation, true long-press
duration reporting, and per-app context are out of scope for v1 and depend on
future daemon work (tracked separately as v2).

## Goals

- Tray icon + on-screen overlay driven purely by existing `Rotate`/`Press`
  signals.
- A Surface-Dial-style radial menu (press-and-hold to open, rotate to
  select, press to confirm) offering a fixed set of four functions: **system
  volume, screen brightness, scroll, and media playback (seek)**.
- Quick rotate (no hold) adjusts whichever function is currently active,
  showing a lightweight HUD. Volume is the default active function until the
  user picks something else via the radial menu; the last pick is remembered
  across restarts.
- Tray menu: enable/disable toggle (pause the gadget's effect on the system
  without stopping the daemon) and quit.
- As broadly usable across Linux distros as reasonably achievable.

## Non-goals (deferred to v2, daemon-dependent)

- Per-app context (different function/behavior depending on focused app).
- True speed-sensitive rotation (requires the daemon to report rotation
  velocity, not just direction).
- Native long-press duration reporting from the daemon (v1 achieves
  hold-to-open-menu purely client-side, timing the gap between the existing
  `Press=1` and `Press=0` events — no daemon change needed for this).

## Toolkit decision

**Qt6 + QML**, chosen over GTK3 despite GTK3 being the more universally
preinstalled option on non-KDE distros. This project prioritizes visual
polish for the radial menu/HUD (the whole point of drawing on Surface Dial
and ProArt Creator Hub for inspiration) over minimizing the dependency
footprint. Qt6 is not currently installed in this dev environment and will
need `qt6-declarative-dev` (or equivalent) at build time; end users will need
Qt6 runtime libraries, which is a real cost on distros that don't already
pull them in via KDE. This is accepted as a deliberate trade-off.

## Architecture

Single Qt6/QML process, `openwheel-gadget`, with four internal modules:

- **DBusListener** — connects to the session bus, watches `org.asus.dial`'s
  `Rotate`/`Press` signals, and separately watches whether that service name
  has an owner (via `NameOwnerChanged`) so the gadget can distinguish "daemon
  not running" from "daemon running but idle."
- **DialController** — the interaction state machine. Starts a 400ms timer on
  `Press=1`; if it fires while still down, opens the radial menu immediately
  (matching Surface Dial's feel of not waiting for release). Tracks whether
  the menu is open to route `Rotate` events either to menu-highlight
  navigation or directly to the active function's `adjust()`. Persists the
  last-selected function via `QSettings`.
- **FunctionRegistry + `DialFunction` interface** — each function is a small
  C++ class implementing:
  - `id()`, `displayName()`, `icon()`
  - `adjust(int direction)`
  - `currentValueLabel()` (for HUD text, e.g. "62%")
  - `isAvailable()` (for graceful degradation)

  The registry is just an ordered list of these. **This is the extensibility
  seam**: adding a function later (v2 or otherwise) means writing one new
  class and adding one line to the registry — no other module changes.
- **TrayIcon** — `QSystemTrayIcon` with two menu entries: enable/disable
  toggle, quit.
- **OverlayWindow** — QML, transparent/frameless/always-on-top
  (`Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::Tool`), renders
  either the radial menu (while open) or the lightweight HUD (on quick
  rotate, auto-hiding after ~1.5s of inactivity). Stays input-transparent
  except while actively capturing dial interaction, so it never steals
  keyboard/mouse focus from the app underneath.

## Function implementations (v1 fixed set)

| Function | Mechanism |
|---|---|
| Volume | `pactl set-sink-volume @DEFAULT_SINK@ ±5%` via `QProcess` — works against both PulseAudio and PipeWire's pulse-compat layer. |
| Brightness | `org.freedesktop.login1.Session.SetBrightness` over D-Bus — permission-free, no udev/group setup required, more portable than shelling out to `brightnessctl`. |
| Scroll | X11: link `libXtst` directly (no `xdotool` dependency) to synthesize scroll-wheel button events. Wayland (best-effort, not a v1 blocker): a `uinput` virtual device, feature-detected at startup; if `uinput` access fails, the Scroll entry is disabled with a one-time notification pointing to setup docs. |
| Media | MPRIS (`org.mpris.MediaPlayer2.*`) over D-Bus; each rotate tick seeks ±5s in the current track. Chosen over next/prev-per-tick because it feels closer to scrubbing, and over continuous seek because the daemon doesn't report rotation speed (v2 concern). |

X11 is the default target for Scroll (zero setup, works everywhere X11
runs); Wayland support is additive, not required for v1 to ship, since
Wayland is not universally used or even available across distros (many
DEs/WMs — XFCE, LXQt, MATE, most tiling WMs — remain X11-primary, and
NVIDIA users frequently still run X11).

## Data flow

1. Daemon emits `Rotate(±1)` / `Press(1|0)` on the session bus.
2. `DBusListener` receives the signal, forwards to `DialController`.
3. On `Press=1`, a 400ms hold timer starts. If it fires before `Press=0`
   arrives, the radial menu opens immediately.
4. While the menu is open: `Rotate` moves the highlighted selection. The
   `Press=0` (release) of the same continuous hold confirms the highlighted
   function as the new active function, persists it via `QSettings`, and
   closes the menu — one continuous down-hold-rotate-release gesture, no
   second press needed.
5. While the menu is closed: `Rotate` calls `activeFunction.adjust(direction)`
   directly and briefly shows the HUD (auto-hides after ~1.5s idle).

## Error handling

- Daemon not running / `org.asus.dial` has no owner → tray icon shows a
  distinct "disconnected" state, updated live via `NameOwnerChanged` (not
  inferred from signal silence, which would be ambiguous with "daemon
  running but idle").
- A function's backend is unavailable (no backlight device, no MPRIS player,
  no X11 session detected) → `isAvailable()` returns false, the entry shows
  disabled/grayed in the radial menu, and `adjust()` is a no-op that logs a
  warning rather than crashing.
- Wayland `uinput` permission failure is caught once at startup via feature
  detection, not surfaced repeatedly on every scroll attempt.

## Testing

- Qt Test unit tests for `DialController`'s timing state machine (rotate-only
  vs hold-opens-menu) and `FunctionRegistry` selection logic, with
  fakes/mocks for process-spawning and D-Bus calls so tests don't depend on
  real audio/brightness/MPRIS state.
- Full end-to-end behavior depends on real HID hardware, the daemon, and live
  system audio/brightness/media state — this cannot be meaningfully
  automated and will be verified manually on real hardware.
- For manual testing without the physical dial in hand, the existing D-Bus
  signals can be driven directly, e.g.:
  `dbus-send --session /org/asus/dial org.asus.dial.Rotate int32:1`
  `dbus-send --session /org/asus/dial org.asus.dial.Press int32:1`
  This will be documented so contributors can exercise the overlay/menu
  without owning the hardware.
