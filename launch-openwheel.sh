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
HIDRAW_DEVICE="/dev/hidraw2"
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

echo "== openwheel launcher =="

# 1. Build the daemon if it's missing.
if [ ! -x "$DAEMON_BIN" ]; then
    echo "Building openwheel-daemon..."
    (cd "$DAEMON_DIR" && cmake . && make)
fi

# 2. Build the gadget if it's missing.
if [ ! -x "$GADGET_BIN" ]; then
    echo "Building openwheel-gadget..."
    mkdir -p "$GADGET_DIR/build"
    (cd "$GADGET_DIR/build" && cmake .. -DCMAKE_BUILD_TYPE=Release && cmake --build .)
fi

# 3. Ensure a daemon owns org.asus.dial, starting one if needed.
if service_registered; then
    echo "org.asus.dial is already owned on the session bus — reusing the running daemon."
else
    echo "No daemon currently owns org.asus.dial."

    if [ ! -e "$HIDRAW_DEVICE" ]; then
        echo "ERROR: $HIDRAW_DEVICE not found. Is the Asus Dial plugged in?" >&2
        echo "(openwheel-daemon currently hardcodes this device path.)" >&2
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
