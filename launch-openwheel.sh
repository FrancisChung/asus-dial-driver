#!/usr/bin/env bash
# Launches openwheel-daemon (asus_wheel) if it isn't already running, waits for it
# to register on the session D-Bus, then launches openwheel-gadget in the foreground.
#
# Safe to run repeatedly: if a daemon (this repo's or any other) already owns
# org.asus.dial on the session bus, this script reuses it instead of starting a
# second one. Only cleans up a daemon it started itself.
set -euo pipefail

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

# 1. Build the daemon. cmake/make are incremental — this is a fast no-op if
#    nothing changed, so it always picks up source edits without needing a
#    manual rebuild.
echo "Building openwheel-daemon..."
(cd "$DAEMON_DIR" && cmake . && make)

# 2. Build the gadget. Same incremental-build reasoning as above.
echo "Building openwheel-gadget..."
mkdir -p "$GADGET_DIR/build"
(cd "$GADGET_DIR/build" && cmake .. -DCMAKE_BUILD_TYPE=Release && cmake --build .)

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
    if [ ! -r "$HIDRAW_DEVICE" ]; then
        echo "ERROR: $HIDRAW_DEVICE exists but isn't readable by $(whoami)." >&2
        echo "Try: sudo usermod -aG input \$USER (then log out/in), or run this script with sudo." >&2
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

# 5. Launch the gadget in the foreground. When it exits, cleanup() stops the
#    daemon above only if this script started it.
echo "Starting openwheel-gadget..."
"$GADGET_BIN"
