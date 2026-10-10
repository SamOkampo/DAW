# Phase 16.2 — Graphite and Muted Amber Theme

Status: IMPLEMENTED IN A REVIEW BRANCH; NOT MERGED; PHYSICAL GUI QA PENDING.

Five files updated: app/JuceTheme.hpp, app/juce_main.cpp, app/JuceSampleBrowser.hpp, app/JuceStepSequencerSurface.hpp and app/JuceAutomationAssistSurface.hpp.
Changes: graphite canvas/panels, warm amber controls, neutral labels, green play/safe-meter indicators, red clip warnings, subtle gradients and smaller panel corners. Browser, sequencer, shell, mixer decorative accents and automation report use centralized semantic tokens instead of older purple/cyan constants.
No AudioEngine, plugin state, realtime callback or .flow v11 model changes.

## Design tokens

| Role | Hex |
| --- | --- |
| Canvas | #101113 |
| Panel | #181A1D |
| Raised | #22252A |
| Hover | #2D3137 |
| Border | #363A40 |
| Primary text | #ECECE8 |
| Secondary text | #B6B5AE |
| Muted text | #92918B |
| Amber | #C2783D |
| Dark amber | #7D4E2C |
| Bright amber | #E09755 |
| Play / safe meter | #76AD76 |
| Clip / danger | #BE6660 |
| Focus | #E9B179 |

## Contrast (calculated, not screenshot tested)

On panel #181A1D: primary text 14.72:1, secondary 8.48:1, muted 5.52:1, focus 9.16:1. Light text on dark amber #7D4E2C is 5.91:1. WARNING: light text directly on bright amber #C2783D would only be 2.93:1, so selected fills should use dark amber; green Play fills are darkened behind labels. Runtime gradients and disabled states still require review.

## Required validation

- [x] Updated semantic theme and key studio surfaces in a separate review branch.
- [x] Source diff limited to UI presentation and this audit note.
- [ ] Full CI at the exact PR head, across supported platforms.
- [ ] Installed Windows screenshots: 100%, 125%, 150% DPI, across transport, Browser, Arrangement, Mixer, Sequencer, Piano Roll, FX rack and dialogs.
- [ ] Audit any remaining hardcoded colors and test hover/active/disabled text contrast.
- [ ] Explicitly reconcile old PR #164 indigo/mint changes with new requested graphite/amber colors; do not silently merge the older theme.
- [ ] Explicitly reconcile PR #163 adaptive window source overlap and preserve Phase 15 RC version macro from PR #161.
- [ ] Keep issue #162 external signing/licence and physical Windows/macOS/Linux release QA gates open.

This theme slice is not the entire Phase 16 layout redesign, plugin docking implementation, or a final 1.0 release.
