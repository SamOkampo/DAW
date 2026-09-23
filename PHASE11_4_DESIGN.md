# Phase 11.4 Design — Mixer + Plugin Workflow

**State: DESIGNED — implementation must start only after this design PR is green and merged.**

Phase 11.4 productizes FLOWDAW's existing Track / Bus / Master mixing and plugin-rack capabilities as a coherent professional desktop workflow. This is a UI/UX phase: it must not change DSP merely to simplify presentation and must preserve the immutable realtime graph, PDC, plugin state model, Undo/Redo semantics and `.flow` v11 compatibility.

## Product objective

A producer should be able to understand and operate the selected Track, Bus or Master without reading implementation-oriented controls. The Mixer and insert rack should communicate signal hierarchy, level state, routing and plugin order at a glance while retaining FLOWDAW's distinctive music-software identity.

The visual direction must remain recognisably FLOWDAW: dense dark production surfaces, intentional rose/indigo/aqua accents, restrained glow/depth, distinctive control silhouettes and strong metering contrast. Do not turn the Mixer into a generic SaaS card grid or web dashboard.

## Existing capabilities that must be preserved

The current JUCE Studio already provides:

- synchronized Track / Bus / Master mixer targets;
- volume, pan, mute and solo editing;
- live target metering;
- Track output routing to Master or Bus;
- Track sends with gain, enable and pre/post state;
- synchronized Mixer ↔ plugin-rack target selection;
- ordered Track / Bus / Master plugin racks;
- built-in Gain, Soft Clip and Width inserts;
- scanned external insert selection and editor opening;
- insert enable/disable, bypass/activate, wet/dry, reorder and remove;
- persistent plugin state through the existing project/plugin model;
- immutable published realtime graph and PDC architecture.

11.4 must productize those capabilities rather than replace their state model.

## Non-negotiable invariants

- Production path remains JUCE 9.0.2.
- `.flow` remains v11 unless an explicitly approved persisted-state requirement appears; none is expected for this phase.
- No filesystem, locks, logging, UI, plugin scan/instantiation, waits or new allocation/preparation work may be introduced in the audio callback.
- Plugin scanning/instantiation/editor/state work remains outside realtime.
- Existing routing/PDC and Track / Bus / Master graph behavior remain unchanged.
- Existing plugin identity, ordering, opaque state and enable/bypass/wet semantics remain unchanged.
- Existing Undo/Redo publication semantics remain unchanged.
- No mastering-DSP claim is created by this UI phase.
- Small focused PRs; required CI green before merge.

## Dependencies

- Phase 10 Mixer/routing/plugin workflow and regression coverage: DONE.
- Phase 11.1 design system: DONE.
- Phase 11.2 shell/transport: DONE.
- Phase 11.3 Arrangement + Browser: DONE after closure PR #94 / CI #300.
- Existing `Project` / immutable RenderGraph / PDC / plugin-host architecture remains authoritative.

## Implementation sequence

### 11.4.1 — Mixer visual hierarchy and target state

Scope only the Track / Bus / Master mixing surface.

Acceptance criteria:

1. Track, Bus and Master have unmistakable visual identities without changing their model or routing semantics.
2. Active target is immediately visible and synchronized with the existing rack target contract.
3. Volume, pan, mute and solo form a coherent channel-strip hierarchy instead of an implementation-oriented row of controls.
4. Metering is visually dominant enough for mixing decisions; meter labels/state remain readable without claiming new measurement standards.
5. Target name/type and live meter belong to one clear mixing surface.
6. Existing control values, callbacks, Undo/Redo and graph publication remain unchanged.
7. No routing/send or plugin-rack redesign is included in this block.

Verification:

- existing core and JUCE tests;
- Track / Bus / Master target switching smoke;
- volume/pan/mute/solo edit + Undo/Redo smoke;
- live meter target follows selected mixer target;
- static review confirms no AudioEngine/serializer/plugin-host diff.

### 11.4.2 — Routing and sends presentation

Scope only existing routing/send controls.

Acceptance criteria:

1. Track output destination is visually grouped with the active Track signal path.
2. Send destination, gain, enabled state and pre/post mode are readable as one send object.
3. Empty/no-bus/no-send states are explicit and non-destructive.
4. Existing add/update/remove send and output-routing semantics are preserved.
5. Bus/Master targets do not expose invalid Track-only routing controls.
6. No routing algorithm, PDC or project-schema change.

Verification:

- route Track Master ↔ Bus;
- create/update/remove send;
- send enable/gain/pre-post smoke;
- save/reopen existing persisted routing state;
- CI and diff-boundary review.

### 11.4.3 — Plugin-rack workflow and insert state

Scope only presentation/interaction over the existing rack model.

Acceptance criteria:

1. Insert order reads vertically or otherwise as an unambiguous signal chain.
2. Each selected insert clearly exposes ACTIVE/BYPASS/DISABLED state, wet/dry and supported parameter/editor action.
3. Built-in and external plugins are visually distinguishable without changing plugin identity/state.
4. Add/reorder/remove operations remain deterministic and Undo/Redo coherent.
5. Mixer target and rack target remain synchronized.
6. External plugin editor/state behavior remains owned by the existing host path; no scan or instantiation is moved to realtime.
7. Empty-rack and unavailable-editor states are explicit.

Verification:

- Track / Bus / Master rack targeting;
- add built-in and scanned external inserts;
- enable/bypass/wet/reorder/remove;
- external editor reopen/state round-trip where fixture is available;
- save/reopen rack order/state;
- existing VST3/AU integration matrix.

### 11.4.4 — Mixer/plugin integration regression and closure audit

No feature expansion. Audit the integrated 11.4 diff.

Required closure review:

- architecture and state ownership;
- realtime-safety contract;
- routing/PDC preservation;
- plugin state/order persistence;
- `.flow` v11 compatibility;
- Undo/Redo regression;
- Track / Bus / Master target consistency;
- FLOWDAW visual-identity compliance;
- Linux / Windows / macOS required CI;
- production golden-path update for the productized Mixer/rack workflow.

11.4 may be marked DONE only after this audit and its closure PR are green and merged.

## Explicitly out of scope

- new EQ/compressor/limiter/mastering DSP;
- LUFS/EBU R128/true-peak algorithm changes or professional-meter certification;
- new routing topology, sidechain architecture or PDC algorithm;
- plugin sandbox architecture changes;
- Piano Roll, Sequencer, Sampler or Automation redesign (11.5);
- general adaptive panel/accessibility system (11.6);
- plugin scanning/device/settings relocation and export/dialog productization (11.7);
- final cross-workflow visual regression (11.8).

## Professional-claim boundary

11.4 can make the existing mixing workflow coherent and production-oriented, but it must not be used to claim FLOWDAW is ready for professional mastering. That claim remains blocked on later DSP, professional measurement, render/export, stability and end-to-end audit work.

## Next permitted implementation

After this design PR passes required CI and is merged, create a fresh branch from `main` and implement **only 11.4.1 — Mixer visual hierarchy and target state**.