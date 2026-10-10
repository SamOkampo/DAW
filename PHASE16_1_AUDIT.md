# FLOWDAW Phase 16.1 — UI Control Inventory and Baseline Audit

Status: **IN PROGRESS — source audit and migration mapping recorded; physical UI baseline still pending**.
Source snapshot: `main` at `5135d6ee32773c6c20d2b8f0272da91e37a92619`, plus Phase 16 documentation branch. This audit is a source review, **not** an installed-app screenshot or a completed physical QA test.

## Counted controls and current geometry

Directly counted in `app/juce_main.cpp` (declaration at approximately line 1036):
- 65 `juce::TextButton` members.
- 14 `juce::ComboBox` members.
- 7 `juce::Slider` members.
- 2 `juce::TextEditor` members.
- 1 `juce::ToggleButton` member.
- **89 widgets across those five types**. This total excludes labels, meters, panel controls and JUCE components inside subviews, so it is **not** the total number of app widgets.

`MainComponent::resized()` (approximately lines 301–388) allocates sequential full-width rows to project actions, transport, editor tabs, mixer/status, plugin scan, track/instrument, native preset, insert rack, sample/chop actions and recording. The minimum 1180px shell provides 1148px content width. Static nominal button widths from the existing row allocations:

| Row | Nominal allocated widths | Baseline observation |
| --- | ---: | --- |
| Project header | 1077px | Dominated by project actions |
| Transport | 544px | Mostly compact already |
| Workspace selector | 1140px | Just 8px spare on a minimum-width content region |
| Plugin scan | 985px | Permanently consumes a full row |
| Track/instrument | 1029px | Permanently consumes a full row |
| Sample/chop | 1054px | Sample commands and export share one row |
| Recording/feel | 1126px | Chops, audio capture and pad choke share one row |

These figures come from `removeFromLeft(...)` arguments, not pixel measurements of a rendered build. Existing PR #163 changes outer viewport sizing and the Browser toolbar. Rebase or reconcile before applying a new layout.

## Confirmed source issues / opportunities

1. **P1 — Cognitive overload and competing persistent actions.** Plugin search, instrument, rack, sampling, audio recording and pad tuning all appear in globally allocated rows even though most actions have a narrow editor context. New design must route each action somewhere before removing a row.
2. **P1 — Four historical rack-add buttons have callbacks but no `setBounds()` in the main layout:** `addRackGain_`, `addRackClip_`, `addRackWidth_`, `addRackExternal_` (constructor ~158–161). These may be legacy, superseded by `nativePluginChoice_`/`addRackNative_` or an existing FX workflow; verify all their actions are represented in the new contextual menu before removing members. Do not assume these controls are user-accessible.
3. **P1 — `exportStems_` is intentionally invisible** (constructor ~142 and `setBounds({})` in layout); it has no onClick callback. Master mix and stems are instead routed via the `showExportMenu()` action from `exportMix_`. Preserve that workflow; do not revive the dead button.
4. **P1 — External editors currently use a separate `PluginEditorWindow : juce::DocumentWindow`** (~95–110), including `createEditorIfNeeded()`, owner and close management. They cannot be moved into the workspace without an editor-lifecycle design and real VST3/AU tests. Phase 16.5 owns the change; support a controlled floating editor fallback.
5. **P1 — Command palette coverage is partial.** `runWorkflowCommand()` maps New/Open/Save/Import, Play, Undo/Redo and five editor tabs (~510–515). Export, recording, plugin maintenance, mixer focus and many contextual operations are not present in this switch. They must remain accessible in future menus, inspector, and command palette.
6. **P2 — Characters and font fallback.** Source includes Unicode bullets/arrows/ellipsis and meaningful graphical symbols, and the user reported malformed characters in the installed Windows build. Validate actual glyph rendering and font/UTF-8 fallback at 100/125/150% scaling before replacing any symbols. Source-only correct UTF-8 does not prove the packaged Windows UI is correct.
7. **P2 — Fragile near-minimum geometry.** The top workspace row allocates 1140px from a 1148px minimum content region. Add responsive overflow/collapse tests that run under `NDEBUG`, reusing the improvement from PR #163 instead of creating a separate partial geometry system.
8. **P2 — Existing purple/cyan decorative colors remain in `main`.** The proposed #164 design is indigo/mint, which the owner has since superseded with graphite/amber. Do not merge an older theme while claiming it implements the new user request.

## Control migration contract

Source of truth for individual controls: `PHASE16_1_CONTROL_MAP.csv`. Each row includes widget identity, current handler/geometry discovery, target design location, and status.

| Current group | Target in the redesigned DAW | Important preserved actions |
| --- | --- | --- |
| Project header | File menu + compact Save/project identity | New template, Open, Save, Import |
| Transport | Compact top bar | Play/Pause, Stop, BPM, Undo/Redo, record state |
| Editor switcher | Center workspace tabs + View menu | Arrangement, Piano Roll, Sequencer, Automation, Sampler |
| Plugin discovery | Left Browser / Tools > Plugins | Search, type filter, preview and scan |
| Track/instrument | Selected track Inspector | Instrument assign/clear, add track/master FX |
| Native inserts/presets | Mixer FX rack / Insert panel | Add FX, preset select/apply, wet and parameters |
| Sample/chop | Sampler/Chops contextual toolbar | Analyze, Chop 8/Auto/Beat/Bar, Match BPM, pads |
| Recording/takes | Transport record + Record/Chops panel | Record audio/chops, monitor, take navigation, quantize/humanize |
| Mixer routing | Docked Mixer with inspector / FX sends | Gain, pan, mute/solo, bus output/send, meter |
| Settings/diagnostics | Tools / Settings dialogs or drawer | Device settings, plugin scan/quarantine, diagnostics |
| Export | File > Export + shortcut | Master and stems export (existing `showExportMenu()`) |

No command is authorized to disappear. Preserve active plugin/track target, update project state via the same handlers, and maintain Undo/Redo and recoverability.

## Evidence matrix and acceptance for 16.1

**Done as source-only audit:**
- [x] Confirmed branch/PR baseline; counted top-level widget classes.
- [x] Identified major toolbar rows, available editor tabs and command-palette pathways.
- [x] Identified four registered rack-add buttons without layout geometry and one intentional hidden export button.
- [x] Described target destinations for all 65 top-level TextButton members in machine-readable map.
- [x] Identified plugin editor lifecycle and pending Phase 16.5 dependency.

**Must be completed with actual installed applications:**
- [ ] Take pre-change screenshots at 100%, 125%, 150% Windows scaling, with actual monitor resolution and build SHA.
- [ ] Verify that glyph issues persist on a clean install and record exact broken strings, expected text and font details.
- [ ] Validate real transport, audio, VST3 editor focus, source-asset browsing, save/reopen and export before removing/moving controls.
- [ ] Validate tab/focus/keyboard traversal and resized 720x520 through 4K windows, including scrolling where necessary.
- [ ] Repeat on macOS/Linux where supported; record real test results, never infer from green CI.

**Next:** Phase 16.2 implementation (graphite/amber semantic theme) in a distinct PR; Phase 16.3 menus and grouped controls after all routing dependencies are mapped and tests are in place. Do not merge Phase 15 RC #161 or claim final 1.0 readiness without issue #162 external evidence. Keep portable `.flow` v11 and realtime audio callback unchanged.
