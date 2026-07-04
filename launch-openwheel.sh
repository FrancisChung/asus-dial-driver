#!/usr/bin/env bash
# Launches openwheel-daemon (asus_wheel) if it isn't already running, waits for it
# to register on the session D-Bus, then launches openwheel-gadget in the foreground.
#
# Run this as your normal user — no sudo, no root, anywhere. The daemon
# needs read/write access to the dial's hidraw device, which comes from
# udev/99-asus-dial-hidraw.rules (install it once, see README) rather than
# from running as root: a root process can't reach your session D-Bus bus,
# PipeWire/PulseAudio, or systemd-logind session anyway (that bus and those
# services are scoped to your user), so running any part of this stack as
# root silently breaks Volume/Brightness/Media (only Scroll, which just needs
# X11, would still work) and even the daemon's own D-Bus signal emission can
# hang or get rejected outright.
#
# Safe to run repeatedly: if a daemon (this repo's or any other) already owns
# org.asus.dial on the session bus, this script reuses it instead of starting a
# second one. Only cleans up a daemon it started itself.
set -euo pipefail

if [ "$(id -u)" -eq 0 ]; then
    echo "ERROR: don't run this script as root/sudo — the dial's hidraw device is" >&2
    echo "group-readable via udev/99-asus-dial-hidraw.rules, so no elevation is needed" >&2
    echo "anywhere, and running as root breaks Volume/Brightness/Media (see comment" >&2
    echo "at the top of this script)." >&2
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DAEMON_DIR="$SCRIPT_DIR/openwheel-daemon"
GADGET_DIR="$SCRIPT_DIR/openwheel-gadget"
DAEMON_BIN="$DAEMON_DIR/asus_wheel"
GADGET_BIN="$GADGET_DIR/build/openwheel-gadget"
DBUS_SERVICE="org.asus.dial"

STARTED_DAEMON=0
DAEMON_PID=""

cleanup() {
    if [ "$STARTED_DAEMON" -eq 1 ] && [ -n "$DAEMON_PID" ] && kill -0 "$DAEMON_PID" 2>/dev/null; then
        echo "Stopping openwheel-daemon (PID $DAEMON_PID)..."
        kill "$DAEMON_PID" 2>/dev/null || true
    fi
}
trap cleanup EXIT

service_registered() {
    dbus-send --session --print-reply --dest=org.freedesktop.DBus \
        /org/freedesktop/DBus org.freedesktop.DBus.NameHasOwner \
        string:"$DBUS_SERVICE" 2>/dev/null | grep -q "boolean true"
}

# hidraw numbering depends on USB/I2C enumeration order (docks, hubs, etc.
# shift it), so find the dial by name instead of assuming a fixed path.
# Mirrors find_hidraw_device() in openwheel-daemon/helpers.c.
find_dial_hidraw_device() {
    local dir uevent
    for dir in /sys/class/hidraw/hidraw*; do
        uevent="$dir/device/uevent"
        if [ -r "$uevent" ] && grep -q "^HID_NAME=.*ASUS2020" "$uevent"; then
            echo "/dev/$(basename "$dir")"
            return 0
        fi
    done
    return 1
}

echo "== openwheel launcher =="

# Rebuild only if a source file is newer than the existing binary (or the
# binary doesn't exist yet). Excludes build/ and CMakeFiles/ since those hold
# cmake's own generated files, whose timestamps don't reflect source changes.
needs_rebuild() {
    local bin="$1"
    shift
    if [ ! -x "$bin" ]; then
        return 0
    fi
    find "$@" -type f \
        \( -name '*.c' -o -name '*.h' -o -name '*.cpp' -o -name '*.hpp' -o -name '*.qml' -o -name 'CMakeLists.txt' \) \
        -not -path '*/build/*' -not -path '*/CMakeFiles/*' \
        -newer "$bin" -print -quit | grep -q .
}

# 1. Build the daemon, but only if its sources changed since the last build.
if needs_rebuild "$DAEMON_BIN" "$DAEMON_DIR"; then
    echo "Building openwheel-daemon (source changed)..."
    (cd "$DAEMON_DIR" && cmake . && make)
else
    echo "openwheel-daemon is up to date, skipping build."
fi

# 2. Build the gadget, same up-to-date check.
if needs_rebuild "$GADGET_BIN" "$GADGET_DIR/src" "$GADGET_DIR/qml" "$GADGET_DIR/tests" "$GADGET_DIR/CMakeLists.txt"; then
    echo "Building openwheel-gadget (source changed)..."
    mkdir -p "$GADGET_DIR/build"
    (cd "$GADGET_DIR/build" && cmake .. -DCMAKE_BUILD_TYPE=Release && cmake --build .)
else
    echo "openwheel-gadget is up to date, skipping build."
fi

# 3. Ensure a daemon owns org.asus.dial, starting one if needed.
if service_registered; then
    echo "org.asus.dial is already owned on the session bus — reusing the running daemon."
else
    echo "No daemon currently owns org.asus.dial."

    HIDRAW_DEVICE="$(find_dial_hidraw_device || true)"
    if [ -z "$HIDRAW_DEVICE" ]; then
        echo "ERROR: No HID device matching ASUS2020 found under /sys/class/hidraw. Is the Asus Dial plugged in?" >&2
        exit 1
    fi
    if [ ! -r "$HIDRAW_DEVICE" ] || [ ! -w "$HIDRAW_DEVICE" ]; then
        echo "ERROR: $HIDRAW_DEVICE exists but isn't read/writable by $(whoami)." >&2
        echo "Install udev/99-asus-dial-hidraw.rules (see README) to grant access, then" >&2
        echo "replug the dial or reboot." >&2
        exit 1
    fi

    echo "Starting openwheel-daemon..."
    "$DAEMON_BIN" &
    DAEMON_PID=$!
    STARTED_DAEMON=1

    echo -n "Waiting for org.asus.dial to register on D-Bus"
    REGISTERED=0
    for _ in $(seq 1 20); do
        if service_registered; then
            REGISTERED=1
            break
        fi
        if ! kill -0 "$DAEMON_PID" 2>/dev/null; then
            echo
            echo "ERROR: openwheel-daemon exited before registering on D-Bus. Check permissions/hardware." >&2
            exit 1
        fi
        echo -n "."
        sleep 0.5
    done
    echo

    if [ "$REGISTERED" -ne 1 ]; then
        echo "ERROR: Timed out waiting for org.asus.dial to register." >&2
        exit 1
    fi
    echo "Daemon is up (PID $DAEMON_PID)."
fi

# 4. Best-effort runtime dependency check for the gadget (non-fatal).
for pkg in qml6-module-qtquick qml6-module-qtquick-window qml6-module-qtqml-workerscript; do
    if ! dpkg -s "$pkg" >/dev/null 2>&1; then
        echo "WARNING: $pkg doesn't appear to be installed — the gadget may fail to load its UI." >&2
    fi
done

# 5. Replace any already-running gadget instead of stacking a second one on
#    top of it — two instances both listen to the same daemon signals and
#    independently track their own active-function/menu state, which is
#    confusing at best (each processes rotate/press events but only one's
#    window is visibly on top), and easy to end up with by just running this
#    script again while an earlier one is still up. The gadget runs as this
#    same user (no root anywhere in this stack), so signaling it needs no
#    special privileges.
OLD_GADGET_PIDS="$(pgrep -f "^${GADGET_BIN}$" || true)"
if [ -n "$OLD_GADGET_PIDS" ]; then
    echo "WARNING: openwheel-gadget was already running (PID $(echo "$OLD_GADGET_PIDS" | tr '\n' ' ')) — stopping it before starting a fresh instance." >&2
    # shellcheck disable=SC2086
    kill $OLD_GADGET_PIDS 2>/dev/null || true
    for _ in $(seq 1 20); do
        pgrep -f "^${GADGET_BIN}$" >/dev/null 2>&1 || break
        sleep 0.2
    done
    if pgrep -f "^${GADGET_BIN}$" >/dev/null 2>&1; then
        echo "WARNING: existing gadget (PID $(echo "$OLD_GADGET_PIDS" | tr '\n' ' ')) didn't exit in time, forcing it." >&2
        # shellcheck disable=SC2086
        kill -9 $OLD_GADGET_PIDS 2>/dev/null || true
    fi
fi

# 6. Launch the gadget in the foreground. When it exits, cleanup() stops the
#    daemon above only if this script started it.
echo "Starting openwheel-gadget..."
"$GADGET_BIN"
