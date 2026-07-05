#!/usr/bin/env bash
# One-time setup: installs the udev rule granting the "input" group
# read/write access to the Asus Dial's hidraw device (root-only by default),
# and adds the invoking user to that group if not already a member. Automates
# the "Permissions (one-time setup)" steps from README.md.
#
# Run as your normal user, not with sudo/root — it calls sudo itself only for
# the specific commands that need it (same ones documented in the README:
# copying the udev rule, reloading udev, and usermod), and needs to know your
# normal user to add to the "input" group.
set -euo pipefail

if [ "$(id -u)" -eq 0 ]; then
    echo "ERROR: don't run this script with sudo/root — it calls sudo itself only" >&2
    echo "for the specific steps that need it (see comment at the top of this" >&2
    echo "script), and needs to know your normal user to add to the 'input' group." >&2
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RULE_SRC="$SCRIPT_DIR/udev/99-asus-dial-hidraw.rules"
RULE_DEST="/etc/udev/rules.d/99-asus-dial-hidraw.rules"
CURRENT_USER="$(id -un)"

if [ ! -f "$RULE_SRC" ]; then
    echo "ERROR: $RULE_SRC not found." >&2
    exit 1
fi

echo "== Asus Dial permissions setup =="

echo "Installing udev rule to $RULE_DEST (needs sudo)..."
sudo cp "$RULE_SRC" "$RULE_DEST"
sudo udevadm control --reload-rules
sudo udevadm trigger --subsystem-match=hidraw
echo "udev rule installed and reloaded."

NEEDS_RELOGIN=0
if id -nG "$CURRENT_USER" | tr ' ' '\n' | grep -qx "input"; then
    echo "$CURRENT_USER is already a member of the 'input' group."
else
    echo "Adding $CURRENT_USER to the 'input' group (needs sudo)..."
    sudo usermod -aG input "$CURRENT_USER"
    NEEDS_RELOGIN=1
fi

echo
echo "Done. If \`ls -la /dev/hidraw*\` doesn't already show 'group input' on the"
echo "dial's device, unplug/replug it (or reboot)."
if [ "$NEEDS_RELOGIN" -eq 1 ]; then
    echo "You also need to log out and back in for your new 'input' group" \
         "membership to take effect."
fi
