#!/usr/bin/env bash
# Installs a per-user XDG autostart entry so the ASUS Dial daemon + gadget
# start automatically when you log in to Linux Mint/Cinnamon (or any other
# XDG-autostart-compliant desktop). User-scoped only — no root, no sudo:
# writes a single .desktop file to ~/.config/autostart/, pointing at this
# repo's launch-asus-dial.sh with its current absolute path.
set -euo pipefail

if [ "$(id -u)" -eq 0 ]; then
    echo "ERROR: don't run this as root/sudo — this installs a per-user autostart" >&2
    echo "entry under ~/.config/autostart for the invoking user." >&2
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LAUNCH_SCRIPT="$SCRIPT_DIR/launch-asus-dial.sh"
AUTOSTART_DIR="$HOME/.config/autostart"
DESKTOP_FILE="$AUTOSTART_DIR/asus-dial.desktop"

mkdir -p "$AUTOSTART_DIR"

cat > "$DESKTOP_FILE" <<EOF
[Desktop Entry]
Type=Application
Name=ASUS Dial
Comment=Starts the ASUS Dial daemon and on-screen gadget
Exec=$LAUNCH_SCRIPT
Icon=input-dialpad
Terminal=false
Categories=Utility;
X-GNOME-Autostart-enabled=true
EOF

echo "Installed autostart entry: $DESKTOP_FILE"
echo "Exec: $LAUNCH_SCRIPT"
echo "It will start automatically on your next login. To remove it, delete that file."
