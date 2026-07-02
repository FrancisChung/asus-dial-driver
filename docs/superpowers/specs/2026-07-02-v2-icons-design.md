# v2-icons Design

## Context

`openwheel-gadget` v1 shipped its radial menu and HUD as text-only (`docs/superpowers/specs/2026-07-01-openwheel-gadget-v1-design.md`, Task 10 of the v1 plan) — a deliberate decision made because QML has no built-in way to render freedesktop icon-theme names, and building that (a `QQuickImageProvider`) was new architecture outside v1's scope. That decision explicitly left the door open for icon rendering as a clean follow-up, since `DialFunction::iconName()` and `DialController::iconNameAt(index)` were already fully wired through and simply unused by QML.

The user has now used v1 on real hardware and wants the radial menu's 4 boxes to show icons instead of text labels, inspired by ASUS's own ProArt Creator Hub radial menu (reference: https://www.asus.com/us/support/faq/1046611/) — a dark circular menu with thin-line minimalist icons (sun, grid, speaker, layered-square, rotate arrows) in a gold/tan color, with the center of the menu showing the current selection as text.

## Goals

- Replace the 4 radial menu boxes' text labels with hand-drawn, minimalist thin-line icons matching the reference's visual style and gold/dark color scheme already established in v1's branding pass:
  - **Volume** → speaker + soundwave glyph
  - **Brightness** → sun with rays
  - **Scroll** → stacked up/down chevrons
  - **Media** → play triangle + pause bars
- Icons must recolor the same way the boxes already do on highlight (gold-on-dark for unselected, dark-on-gold for the highlighted/selected box), matching v1's existing inversion behavior exactly.

## Non-goals (explicitly out of scope for this branch)

- The "ASUS Dial" center-of-menu logo (shown while the menu is open) stays text, unchanged from v1.
- The HUD confirmation popup (shown after releasing to confirm a selection, e.g. "Scroll") stays text, unchanged from v1.
- No C++ changes. `DialController::iconNameAt(index)` already exists and already returns a stable per-function id string (`"audio-volume-high"`, `"display-brightness"`, `"input-mouse"`, `"media-playback-start"`) — this branch only adds a QML consumer for it.
- No new build dependencies (no Qt SVG image loading, no shader/effects modules) — icons are drawn as native `QtQuick.Shapes` geometry, matching the same "no new infrastructure" constraint v1 held to for the branding pass.

## Architecture

Pure QML addition. A new reusable component, `openwheel-gadget/qml/DialIcon.qml`, takes an `iconId: string` and a `color: color` property and renders the matching glyph as thin-line `Shape`/`ShapePath` geometry (~1.5-2px stroke, no fill). `RadialMenu.qml`'s existing delegate (currently a `Rectangle` containing a `Text` label) has its `Text` replaced with a `DialIcon`, reusing the delegate's already-existing `index === dialController.highlightedIndex` check to pick the icon's color exactly the way it already picks the box's background/border color — no new state, no new bindings beyond what's already there.

## Components

**`DialIcon.qml`** (new):
- Properties: `iconId: string` (one of the four known ids above), `color: color` (defaults to the v1 gold, `#C9A87C`).
- Internally: four sibling `Item`s, each gated by `visible: iconId === "<id>"`, each containing one `Shape` with the glyph's path geometry:
  - *Volume*: a small trapezoid (speaker cone) plus two short concentric arcs to its right (soundwaves).
  - *Brightness*: a small circle plus 8 short radiating line segments around it.
  - *Scroll*: two stacked chevrons (a `^` shape above a `v` shape).
  - *Media*: a small right-pointing triangle (play) plus two short vertical bars beside it (pause).
- All four glyphs are simplified line-art approximations sized to a common ~28×28 logical bounding box, inspired by the reference image rather than pixel-perfect copies of it (the reference's icons are for different functions in a different app; ours are original simple shapes in the same minimalist style).
- If `iconId` doesn't match any of the four known ids, nothing renders (no crash, no placeholder glyph) — this can't happen in practice given the fixed 4-function registry, but keeps the component safe against a future function id it doesn't yet know about.

**`RadialMenu.qml`** (modified): the delegate's `Text` element is removed; a `DialIcon { anchors.centerIn: parent; iconId: dialController.iconNameAt(index); color: index === dialController.highlightedIndex ? "#1a1a1a" : "#C9A87C" }` takes its place. The delegate's existing `x`/`y` circular-layout math, `opacity` (live-availability) binding, box `color`/`border` properties, and overall structure are otherwise unchanged.

## Data flow

Unchanged from v1 — `dialController.iconNameAt(index)` was already available and correct since Task 8; this branch is purely a new QML consumer of an existing, already-tested C++ API.

## Error handling

Covered under Components above: an unrecognized `iconId` renders an empty `DialIcon` rather than crashing or showing a broken/missing-glyph indicator.

## Testing

Consistent with v1's established convention for QML-only changes (Task 10, and the ASUS Dial branding pass): QML visuals are not unit tested in this project — there is no automated way to assert on rendered glyph appearance. Verification is: the project builds cleanly, and the user visually confirms the icons render and recolor correctly by running the app.
