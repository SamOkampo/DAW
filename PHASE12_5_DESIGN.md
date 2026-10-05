# Phase 12.5 Design — Native-content starter kits/templates

**State: IMPLEMENTATION CONTRACT**

Phase 12.5 upgrades the existing Blank / Boom Bap / Trap / Lo-Fi project templates so the musical starters use FLOW Core samples and native instrument presets without persisting machine-specific install paths.

## Compatibility contract

- Portable project format remains **.flow v11**.
- Existing `SampleAsset::nativeKey` is reused; no new serialized field is added.
- Legacy built-in drum keys such as `kick`, `snare` and `hat` keep their current behavior.
- First-party library assets use a namespaced stable reference: `content:<stable-content-id>`, for example `content:flow.kick.deep`.
- A project containing bundled content must keep `SampleAsset::path` empty for that asset. Absolute build/install paths must never be serialized merely because a template used FLOW Core.
- New binaries must continue loading existing v1-v11 projects exactly as before.

## Resolver boundary

Project loading gains an optional first-party content resolver/context. Serialization remains responsible for reading the stable `nativeKey`, but installation-path knowledge stays outside the project document.

Resolution order for root samples when `loadAudio=true`:

1. `nativeKey` starts with `content:` → ask the supplied resolver for that stable content ID.
2. non-empty legacy `nativeKey` → current `makeNativeDrum` behavior.
3. empty `nativeKey` + existing user path → read WAV from the user path.

If a `content:` reference cannot be resolved, project structure still loads; the audio asset is reported/missing rather than silently substituting an unrelated sound. Phase 12.6 owns robust installed/package root discovery.

No resolver/file I/O may execute from the realtime audio callback.

## Starter templates

### Blank
- 120 BPM.
- No bundled assets required.

### Boom Bap
- 90 BPM, groove/swing suited to the existing template.
- FLOW Core: `flow.kick.deep`, `flow.snare.dust`, `flow.hat.tight`.
- Native preset: `flow.preset.keys.dark`.
- Existing editable sequencer + melodic-pattern structure remains.

### Trap
- 140 BPM.
- FLOW Core: `flow.kick.punch`, `flow.clap.snap`, `flow.hat.tight`.
- Native preset: `flow.preset.808.dirty` or another catalog preset chosen explicitly by the template contract.
- Faster hat pattern may differ from Boom Bap, but all edits remain normal Project state.

### Lo-Fi
- 82 BPM with the existing stronger swing baseline.
- FLOW Core: `flow.kick.deep`, `flow.snare.dust`, `flow.hat.open`.
- Native preset: `flow.preset.pad.dust`.
- No hidden processing or mastering claim.

## Template builder API

The template builder should live in the core/domain layer rather than being hard-coded only in JUCE UI.

Inputs:
- template kind;
- FLOW Core root/catalog or a small content accessor abstraction;
- project sample rate.

Outputs:
- normal editable `Project` with in-memory audio hydrated for bundled sample assets;
- stable `content:` native references;
- instrument state copied from the selected `.flowpreset`;
- no absolute bundled-content path persisted.

The JUCE New Project menu calls this builder. If FLOW Core is genuinely unavailable before Phase 12.6 discovery closes, the UI may fall back to the existing legacy generated starter rather than crash.

## Required tests

1. Each template builds with expected BPM, sample IDs, lanes and native preset role.
2. Every bundled sample has empty persisted `path` and a `content:` stable reference.
3. Save template to `.flow`; serialized text must not contain the temporary/install FLOW Core root.
4. Reopen using a resolver; all bundled sample audio is rehydrated and non-empty.
5. Reopen without resolver; document/arrangement state remains loadable and bundled samples remain identifiable as missing rather than causing an unknown-native-drum exception.
6. Existing legacy native-drum projects still reload with audio.
7. Project format remains 11 after roundtrip.
8. Realtime/offline rendering parity tests remain unchanged; template construction/loading is control/filesystem work only.

## Phase boundary

12.5 does not solve package-location discovery. It consumes an explicit content resolver/root. **12.6** replaces build-root assumptions with reliable Linux/Windows/macOS installed/package discovery and validates the same templates from installed artifacts.
