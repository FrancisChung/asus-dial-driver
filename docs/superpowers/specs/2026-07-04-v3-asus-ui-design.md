# v3-asus Design

## Context

The user wants `openwheel-gadget`'s on-screen UI restyled to mimic ASUS's own "ASUS Dial and Control Panel" software (reference: https://www.asus.com/proart/software-solutions/asus-dial-and-control-panel/, plus a user-supplied screenshot and screen recording of it in use inside Adobe Lightroom).

What the reference shows:
- A marketing screenshot: a full ring divided into 7 solid pie-slice wedges (no gaps between them), each with a thin white-line icon (one wedge unlabeled/blank), one wedge highlighted in cream/tan against otherwise dark-navy/black wedges, and a large center disc showing the active function's name as text (e.g. "System Volume").
- A screen recording: a smaller, compact floating version of the same ring sitting over an app window while actively rotating, with a center label (e.g. "Exposure") and what looks like a brief swirl/transition animation.

v2's existing color palette (dark navy + cream/tan accent, established in the v1 branding pass) already closely matches ASUS's reference. This is a **geometry and component** change, not a re-theme.

## Goals

- Redesign `RadialMenu.qml` from its current v2 look (separate floating circular buttons around a center hub) into a solid pie-ring: continuous wedges with no gaps, matching the reference's silhouette.
- Show 7 wedges total, matching the reference's wedge count: the 4 real functions (Volume, Brightness, Scroll, Media) plus 3 permanently-disabled/dimmed placeholder wedges with no icon and no bound function, purely for visual parity. Rotating skips over placeholders straight to the next real function.
- Add a new compact floating dial (`CompactDial.qml`) that replaces `Hud.qml` for quick-rotate feedback, styled as a small ring/arc segment with a center value label, matching the compact Lightroom-recording look, rather than today's plain text HUD box.
- Reuse the existing color tokens (cream/tan `#C9A87C`, dark navy background) already established in v1/v2 — no new palette.

## Non-goals (explicitly out of scope for this branch)

- No changes to `DialController`, `FunctionRegistry`, `DialFunction`, the daemon, or any D-Bus/backend plumbing. This is QML-only.
- No new functions or per-app context (that's the previously-discussed, explicitly bigger "option 3" — app-context tracking, profile persistence, a separate settings/Control Panel window — deferred to its own future brainstorm, not part of v3-asus).
- No swirl/transition animation cloning from the reference video — not called out as a goal by the user; can be revisited after the prototype is tested live if it's missed.
- No new build dependencies — `qml6-module-qtquick-shapes` is already a dependency as of v2-icons (for `DialIcon.qml`); this branch reuses it rather than adding anything new.

## Architecture

Pure QML restructuring plus one new reusable primitive. The wedge geometry — for both the full ring and the compact dial — is built from `QtQuick.Shapes`' `PathAngleArc`, computed procedurally from a wedge count/index rather than hardcoded, so the ring isn't tied to exactly 4 or exactly 7 segments.

## Components

**`RingWedge.qml`** (new): a single wedge primitive.
- Properties: `startAngle: real`, `spanAngle: real`, `innerRadius: real`, `outerRadius: real`, `color: color`, `iconId: string` (optional — empty for placeholder wedges).
- Renders one filled pie-slice segment (`Shape`/`ShapePath` using `PathAngleArc` for the outer and inner arcs) plus, if `iconId` is non-empty, a centered `DialIcon` at the wedge's mid-angle/mid-radius.
- Used at large radius/full detail by `RadialMenu.qml` (one wedge per slot, all 7 positions) and at small radius by `CompactDial.qml` (typically just the single highlighted wedge plus a thin background ring).

**`RadialMenu.qml`** (redesigned): replaces the current delegate-per-button layout with a `Repeater` of 7 `RingWedge`s laid out contiguously around the circle (`spanAngle = 360 / 7`). The 4 real wedges bind `iconId`/`color`/highlight exactly as today (via `dialController.iconNameAt(index)` and the existing highlighted-index check); the 3 placeholder wedges get a fixed dim color and empty `iconId`, and are excluded from the highlight/selection index range so rotation skips them. Center disc (`CenterLogo.qml`) keeps showing the active function's name, resized to the reference's proportions.

**`CompactDial.qml`** (new, replaces `Hud.qml`): a small floating ring built from one or two `RingWedge`s (the active function's wedge, highlighted, against a thin dim background ring) plus a center `Text` showing `currentValueLabel()` (e.g. "62%") — same data `Hud.qml` shows today, restyled to match the reference's compact on-screen look instead of a plain text box.

**`DialIcon.qml`, `DialOverlay.qml`**: kept as-is structurally; icon placement math inside wedges (mid-angle/mid-radius) is new, but `DialIcon`'s own glyph rendering is unchanged.

## Data flow

Unchanged from v2. `DialController` still drives the same states (idle → quick-rotate → press-hold menu-open → confirm/close) off the same D-Bus signals; `FunctionRegistry` still exposes the same 4 `DialFunction`s. The only change is which QML component `DialOverlay.qml` instantiates per state (`CompactDial` instead of `Hud` for quick-rotate; the redesigned `RadialMenu` for menu-open) and how many wedge slots that component iterates over (real functions + static placeholder count, placeholders defined QML-side with no backing `DialFunction`).

## Error handling

Placeholder wedges are inert by construction (no `iconId`, not part of the selectable index range) — same "can't crash, nothing to handle" reasoning as `DialIcon.qml`'s unknown-id case in v2. Function-unavailable dimming (e.g. Media with no player running) reuses the exact opacity treatment already in place today, just applied to the new wedge shape instead of the old button shape.

## Testing

No backend logic changes, so all existing C++ tests (`test_dialcontroller`, `test_functionregistry`, `test_volumefunction`, `test_brightnessfunction`, `test_scrollfunction`, `test_mediafunction`, etc.) run unchanged as a regression net — no new unit tests, since there's no new testable backend behavior. Verification is manual: build, launch via `launch-openwheel.sh`, exercise quick-rotate and press-hold-rotate-release with the physical dial.

The user has explicitly said they want to test the built prototype live before deciding whether further visual changes are needed — treat the first implementation as a checkpoint, not a final answer. Once approved, record a `demo-v3-asus.gif` for the README the same way as v1/v2.
