# Phase 11.5 Design — Musical Editors

**State: DESIGNED — implementation must start only after this design PR is green and merged.**

Phase 11.5 productizes FLOWDAW's existing Piano Roll, Step Sequencer, Smart Sampler/REC CHOPS and Automation editing surfaces into one coherent musical-editing language. This is a UI/UX and interaction-consistency phase. It must preserve the existing Project/Pattern/SampleAsset/AutomationLane ownership, Undo/Redo publication path, audio-engine behavior and `.flow` v11 compatibility.

## Product objective

A producer moving between Piano Roll, Sequencer, Sampler and Automation should immediately understand:

- what object is selected;
- what musical grid or time resolution is active;
- which edits affect the selection versus the whole pattern/sample/lane;
- where contextual controls live;
- which state is persistent project data and which state is transient audition/navigation state;
- how keyboard/mouse interaction maps across editors.

The surfaces should feel related without becoming visually identical. FLOWDAW's dark production identity, rose/indigo/aqua accents, selective glow/depth and strong beat/grid contrast remain mandatory. Avoid generic card dashboards, web-form layouts and interchangeable AI-generated control panels.

## Existing capabilities that must be preserved

The current JUCE Studio already provides:

### Piano Roll
- persistent `Pattern::midiNotes` with stable IDs, start, length, pitch and velocity;
- bounded multi-selection and select-all;
- add, move, resize, delete and keyboard nudge;
- grouped velocity/length edits;
- 1/8, 1/16 and 1/32 MIDI grids;
- root/scale guidance and native instrument selection;
- keyboard audition and octave navigation;
- instrument gain/pan/tone/attack/release/drive/delay controls;
- edits committed through the existing Project snapshot/Undo path.

### Step Sequencer
- 16/32/64-step patterns and paging;
- native drum lanes;
- step enable state with Velocity, Probability and Microtiming;
- lane Volume/Pan/Mute/Solo;
- Swing/Humanize and existing groove presets;
- keyboard step navigation/editing;
- persistent pattern state through the existing Project model.

### Sampler / REC CHOPS
- SampleAsset-backed smart sampling and non-destructive slices;
- Equal/Auto/Beat/Bar chopping;
- stable slice identity, Rename/Gain/Pan/Choke;
- multi-bank pad performance and preview;
- Match BPM using existing derived-asset/source-ratio behavior;
- REC CHOPS performance capture and reversible Quantize/Humanize feel editing;
- existing AudioEngine preview/trigger routes outside the realtime callback's UI state.

### Automation
- persistent `AutomationLane` objects;
- Track/Bus/Master volume/pan targets already supported by the engine;
- write point at current playhead, deterministic point normalization and clear-lane operation;
- musical-tick storage with linear interpolation during playback;
- existing Project commit/Undo path.

## Authoritative state ownership

11.5 must not create a second musical-editor model.

- **Piano Roll:** `Pattern::midiNotes`, MIDI grid/scale/default-length and `Pattern::instrument` remain authoritative. Selection, drag and audition state remain UI-transient inside the Piano Roll surface.
- **Sequencer:** existing Pattern step/lane/groove state remains authoritative. Page and focused-step presentation remain UI-transient.
- **Sampler:** existing `SampleAsset`, `SampleSlice`, Pattern chop events and existing sample/chop helpers remain authoritative. Pad/bank focus and preview state remain UI-transient.
- **Automation:** `Project::automation` / `AutomationLane` remain authoritative. Current target choice and selected UI point/inspection state are presentation concerns only.
- **Undo/Redo:** all persistent mutations continue through the existing `CommitFn(Project before, std::string name)` / publication path. No editor-local persistent undo stack.
- **Realtime graph:** editor changes reach audio only through the existing project publication/render-graph mechanisms.

A shared presentation helper may be introduced for visual/interaction grammar, but it must own no Project, Pattern, SampleAsset, AutomationLane, AudioEngine or plugin state.

## Shared musical-editor interaction grammar

All four surfaces should converge on the following presentation contract where applicable:

1. **Selection language**
   - primary selection has a high-contrast focus treatment;
   - grouped/multi-selection is distinct from the primary anchor;
   - audition/trigger state is visually different from persistent selection;
   - empty/no-selection state is explicit.

2. **Time/grid language**
   - bar/beat/subdivision hierarchy is visually readable;
   - active grid resolution is visible without reading implementation details;
   - snapped versus free/transient movement must not be ambiguous;
   - the visual grid may be restyled but musical tick semantics remain unchanged.

3. **Context inspector language**
   - selection-specific values are grouped separately from pattern/sample/lane-wide values;
   - destructive actions are visually separated from audition/navigation;
   - controls preserve existing callbacks and commit boundaries.

4. **Keyboard/focus language**
   - keyboard-focused editor is visibly identifiable;
   - existing shortcuts remain intact;
   - mouse interaction and keyboard interaction must resolve to the same authoritative state.

5. **Visual identity**
   - Piano Roll may emphasize pitch/scale colour bands;
   - Sequencer may emphasize rhythmic cells and lane identity;
   - Sampler may emphasize pads/slice energy and sample context;
   - Automation may emphasize curve/point readability and parameter range;
   - consistency comes from hierarchy, focus, grid and inspector grammar, not from cloning one generic panel across all editors.

## Non-negotiable invariants

- Production desktop path remains JUCE 9.0.2.
- `.flow` remains v11 for Phase 11.5 unless a separately designed and approved migration becomes necessary; none is expected.
- No filesystem, logging, UI work, locks, waits, plugin scanning/instantiation, sample decoding, project copying or new dynamic-allocation work may be added to the audio callback.
- Preview/trigger operations may use the existing AudioEngine paths only; UI code must not migrate engine preparation or file I/O into realtime.
- Existing MIDI scheduling, native-instrument rendering, sequencer timing, sample/chop rendering and automation interpolation semantics remain unchanged.
- Existing Undo/Redo semantics remain unchanged.
- Existing Arrangement/Mixer target synchronization must not regress.
- Phase 11.5 does not create new DSP or professional mastering claims.
- Small focused PRs; required CI must be green before merge.

## Dependencies

- Phase 10 production editing workflows and regression coverage: DONE.
- Phase 11.1 design system: DONE.
- Phase 11.2 shell/transport: DONE.
- Phase 11.3 Arrangement + Browser: DONE.
- Phase 11.4 Mixer + plugin workflow: DONE; status synchronization closed through PR #104.
- Existing `Project`, `Pattern`, `SampleAsset`, `AutomationLane`, AudioEngine and immutable render-graph ownership remain authoritative.

## Implementation sequence

### 11.5.1 — Piano Roll interaction + visual hierarchy

Scope only the existing Piano Roll surface and presentation around its existing controls.

Acceptance criteria:

1. Note selection, primary anchor and grouped selection are visually unambiguous.
2. Pitch rows, root/scale guidance, beat/grid hierarchy and note lengths are readable at a glance.
3. Add/move/resize/delete/nudge behavior and snapping semantics remain unchanged.
4. Grid, scale/root and instrument context are grouped separately from selected-note velocity/length controls.
5. Native instrument shaping controls remain available but no longer compete visually with note editing.
6. Keyboard audition/octave navigation remains intact and focus state is visible.
7. Existing Project mutations and Undo/Redo boundaries are preserved.
8. No MIDI scheduling, instrument DSP, serializer or project-format changes.

Verification:
- add/move/resize/delete one note;
- multi-select, select-all and grouped nudge/velocity/length edits;
- grid change 1/8 ↔ 1/16 ↔ 1/32;
- root/scale and native-instrument change;
- keyboard audition/octave smoke;
- Undo/Redo for representative note and control edits;
- static diff review confirms no AudioEngine/serializer/render-graph behavior change.

### 11.5.2 — Step Sequencer interaction + rhythmic hierarchy

Scope only the existing Step Sequencer surface.

Acceptance criteria:

1. Lane identity, selected lane, selected/focused step and enabled step states are distinct.
2. Bar/beat/page hierarchy remains legible for 16/32/64-step patterns.
3. Velocity, Probability and Microtiming clearly describe the selected step rather than appearing as unrelated global controls.
4. Lane Volume/Pan/Mute/Solo and drum assignment read as lane-level state.
5. Swing/Humanize and groove presets read as pattern-level state.
6. Existing page/length, keyboard navigation/toggle/clear/duplicate and commit semantics remain unchanged.
7. No sequencer timing, probability, microtiming or drum-rendering algorithm changes.

Verification:
- 16/32/64-step and page navigation smoke;
- toggle/edit step Velocity/Probability/Microtiming;
- lane drum assignment and mixer controls;
- Swing/Humanize/preset edit;
- keyboard step navigation/editing;
- Undo/Redo and save/reopen persistence;
- core/JUCE regression coverage.

### 11.5.3 — Sampler / REC CHOPS performance hierarchy

Scope presentation/interaction over existing Smart Sampling, slice and chop-performance behavior.

Acceptance criteria:

1. Active sample, bank, pad/slice selection and currently auditioned pad are visually distinct.
2. Pad/slice identity, name, Gain, Pan and Choke belong to one clear contextual inspector.
3. Equal/Auto/Beat/Bar Chop and Match BPM remain explicit transformation actions and retain existing non-destructive semantics.
4. REC CHOPS record state is unmistakable and visually separated from ordinary preview/audition.
5. Quantize/Humanize/Reset Feel communicate performance-edit scope without changing captured `recordedTick` / `recordedVelocity` source data.
6. Empty sample/no-slice/no-selection states are explicit.
7. No sample decoding, filesystem work, derived-asset generation or AudioEngine preparation is moved into realtime.

Verification:
- import/select existing sample and preview;
- create Equal/Auto/Beat/Bar chops;
- bank navigation and pad trigger;
- Rename/Gain/Pan/Choke edit;
- Match BPM round-trip using existing source/ratio model;
- REC CHOPS capture then Quantize/Humanize/Reset Feel;
- Undo/Redo and save/reopen;
- realtime-safety diff review.

### 11.5.4 — Automation editing hierarchy

Scope the creative Automation portion of the existing Automation/Assist surface. Do not re-redesign Mixer routing or Assist/Health in this block.

Acceptance criteria:

1. Automation target, route/target identity, current value and lane graph form one coherent editing surface.
2. Existing Track/Bus/Master volume/pan targets remain unchanged.
3. Automation points and curve segments have clear selected/normal/read states; value range context is visible.
4. Write-at-playhead and clear-lane actions remain deterministic and Undo/Redo coherent.
5. Musical-time grid/position context is visible without changing tick storage/interpolation.
6. Existing Bus Mixer/send behavior embedded in the current component is not duplicated or semantically changed; later relocation/productization remains 11.7 scope.
7. No new automation target type or playback interpolation algorithm is introduced.

Verification:
- Track volume/pan, Bus volume/pan and Master volume target smoke;
- write points at multiple playhead positions;
- deterministic same-tick normalization;
- clear lane + Undo/Redo;
- save/reopen automation;
- realtime/offline playback regression for existing automation targets.

### 11.5.5 — Musical-editor integration regression + closure audit

No feature expansion. Audit the integrated Phase 11.5 diff.

Required closure review:
- state ownership and absence of duplicate editor models;
- realtime-safety contract;
- MIDI/sequencer/sampler/automation semantic preservation;
- `.flow` v11 compatibility and backward loading;
- Undo/Redo regression across all four editors;
- keyboard focus/interaction consistency;
- Arrangement ↔ selected Pattern/editor synchronization;
- FLOWDAW visual-identity compliance;
- Linux / Windows / macOS required CI;
- production golden-path update covering Piano Roll, Sequencer, Sampler/REC CHOPS and Automation.

11.5 may be marked DONE only after this audit and closure PR are green and merged.

## Tests and validation boundaries

Implementation PRs should prefer presentation/testable seams over broad rewrites. Where a visual helper is introduced, tests should validate deterministic state-to-presentation decisions where practical without requiring screenshot-perfect rendering.

Required regression sources include:
- existing core project/model tests;
- existing MIDI/Piano Roll and sequencer production workflow coverage;
- existing sample/chop persistence/render tests;
- existing automation persistence/playback tests;
- JUCE runtime smoke on Linux, Windows and macOS;
- existing plugin-fixture/package matrix where the repository workflow includes it.

No PR may claim a surface complete merely because it compiles. Representative keyboard/mouse edit paths and Undo/Redo must be verified.

## Explicitly out of scope

- new MIDI CC/MPE architecture;
- new piano-roll quantization/humanization algorithms;
- new sequencer probability/microtiming semantics;
- new sampler time-stretch/pitch-shift DSP or destructive audio editing;
- new automation target families or interpolation modes;
- new synthesis/instrument DSP;
- adaptive/resizable panel system and broad accessibility pass (11.6);
- export/settings/recovery/plugin-scan/diagnostic relocation (11.7);
- final application-wide visual regression and Phase 11 closure (11.8);
- professional mastering certification.

## Professional-claim boundary

11.5 can make FLOWDAW's existing musical editing workflows coherent, faster to read and more production-oriented. It does not validate mastering DSP, professional metering, render/export quality or long-session stability, and therefore cannot be used to declare FLOWDAW ready for professional mastering.

## Next permitted implementation

After this design PR passes required CI and is merged, create a fresh branch from `main` and implement **only 11.5.1 — Piano Roll interaction + visual hierarchy**.