# Phase 11.6 High-DPI / Accessibility Validation

## Scope

Phase 11.6 uses JUCE logical coordinates for window, shell, panel, focus and hit-target geometry. It does not multiply component bounds by an operating-system display scale factor. The deterministic layout seam in `ShellLayout.hpp` therefore receives the same logical dimensions at 100%, 125%, 150% and 200% desktop scaling.

This document is validation evidence for the implementation contract, not an accessibility or screen-reader certification.

## Automated evidence

The Phase 11.6 layout tests cover minimum, compact, default and large logical window sizes and assert that:

- shell regions remain non-negative and inside their parent region;
- Browser width stays within its bounded contract while preserving the creative editor minimum;
- the creative editor retains its minimum logical width/height before secondary panels receive space;
- Mixer and Audio I/O utility regions collapse rather than forcing invalid geometry;
- Automation / Bus / Assist regions are calculated from current logical width rather than fixed absolute columns;
- no platform scale factor is applied inside the deterministic geometry layer.

The required FLOWDAW CI remains the integration gate on Linux, Windows and macOS.

## Representative manual scale checklist

When validating an installed build on a native desktop, check the same workflow at **100%, 125%, 150% and 200%** OS scale:

1. Launch at the default 1440×1040 logical size and resize down to 1180×720.
2. Confirm shell controls, workspace tabs and contextual controls remain reachable.
3. Confirm Browser/editor bounds remain readable and the editor keeps priority over Mixer/Audio I/O.
4. Toggle Mixer and Audio I/O; confirm the restore affordances remain visible.
5. Tab and Shift+Tab through the active workspace; hidden/collapsed controls must not receive focus.
6. Confirm Piano Roll, Sequencer, Sampler and Automation show a non-colour-only focus outline.
7. Type in plugin search and pad-name text fields; global musical/project shortcuts must not fire while typing.
8. Confirm tooltips, labels, slider values and combo-box text remain legible and hit targets remain usable.
9. Open and close a plugin editor; focus should return to the invoking FLOWDAW control where practical.

Native display-scale and assistive-technology inspection is inherently a manual smoke check. Automated CI does not claim physical-pixel, screen-reader or WCAG certification.
