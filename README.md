# asus-dial-driver
An Asus Dial implementation for Linux environment(s). Currently only tested on Linux Mint (Ubuntu)

This is based on a fork of https://github.com/fredaime/openwheel/ with some fixes implemented to get it working.
A new UI has been created to make this project a viable tool for Asus Dial owners on Linux.


## openwheel-gadget (tray + overlay)

Build dependencies (Debian/Ubuntu naming): `qt6-base-dev qt6-declarative-dev libqt6svg6-dev libxtst-dev`.

Runtime dependencies (also needed to actually run the built binary, not just compile it):
`qml6-module-qtquick qml6-module-qtquick-window qml6-module-qtqml-workerscript`. Without these,
the binary builds and links fine but exits immediately with "module ... is not installed" QML
errors as soon as it tries to load `DialOverlay.qml`.

```bash
cd openwheel-gadget
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

Run `./openwheel-gadget` (requires `openwheel-daemon`'s `asus_wheel` running and emitting D-Bus
signals for the dial to actually do anything; the gadget also works with signals sent manually via
`dbus-send`, useful for testing without the physical hardware. The daemon emits `Rotate` (int32 ±1)
and `Press` (int32 1|0) signals on `org.asus.dial` / `/org/asus/dial` on the session bus).

To start automatically with your session, first symlink the built binary to your PATH (if not already
done):
```bash
mkdir -p ~/.local/bin
ln -sf "$(pwd)/openwheel-gadget/build/openwheel-gadget" ~/.local/bin/openwheel-gadget
```

Then copy `openwheel-gadget/openwheel-gadget.desktop` to `~/.config/autostart/`.

**Wayland scroll support (optional):** the Scroll dial function uses X11's XTest extension by
default and works out of the box on any X11 session. On Wayland, scroll instead uses a `uinput`
virtual device, which requires one-time setup: add your user to the `input` group
(`sudo usermod -aG input $USER`, then log out and back in) so the gadget can open `/dev/uinput`
without root. If this isn't set up, the Scroll entry in the radial menu is simply disabled — every
other function works normally on Wayland regardless.

