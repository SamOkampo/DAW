# FLOWDAW Production Golden Path

This is the manual acceptance path for the shipping JUCE Studio. It is intentionally user-facing: the run passes only if a producer can complete a short song workflow from the installed application without opening a debugger, editing a project file by hand, or launching the legacy X11 shell.

## Test build

Use an installed/package artifact produced by the normal production configuration:

- JUCE Studio enabled.
- Legacy X11 not required.
- A working output device.
- An input device/microphone for the recording section.
- One known-good VST3 effect on all platforms; on macOS, also exercise an AU effect when available.
- A short valid WAV loop or one-shot.

Record the platform, artifact/commit, audio device and external plugin used in the test report.

## Pass criteria

The complete run is **PASS** only when every required step below succeeds without a crash, hang, manual project-file edit or fallback to a developer-only tool. A missing optional plugin format may be marked N/A only when the platform does not support that format.

### 1. Launch and starter project

1. Launch the installed FLOWDAW application.
2. Choose **New**.
3. Confirm the starter session contains a usable drum Pattern and FLOW Keys melodic Pattern.
4. Press Play and confirm audible transport playback.
5. Change BPM, stop, play again and confirm the transport follows the new tempo.

Expected: a user can make sound immediately after launch.

### 2. Import and Smart Sampling

1. Select the track that will receive the sample.
2. Import a valid WAV.
3. Confirm the asset appears in the sample selector and an audio clip appears in Arrangement.
4. Run **Analyze** and confirm BPM/beat-grid feedback is produced when the material is analyzable.
5. Exercise **CHOP 8**.
6. Exercise one content-aware chop mode: Auto, Beat or Bar.
7. Drag at least one slice boundary, then Split or Merge a slice.
8. Trigger slices from the visible pads and from the mapped computer keyboard.
9. Rename a pad and change gain, pan and choke.
10. If the detected BPM differs from the project, exercise **Match BPM** and confirm the source asset remains available while the derived asset is used non-destructively.

Expected: importing, analyzing, slicing and performing a sample require no developer workflow.

### 3. REC CHOPS and drums

1. Start **REC CHOPS**.
2. Perform at least four slice hits.
3. Stop recording and confirm a persistent chop Pattern/placement was created.
4. Apply Quantize and Humanize, then use Reset Feel.
5. Open the Step Sequencer.
6. Toggle Kick, Snare and Hat steps.
7. Change at least one step Velocity, Probability and Microtiming value.
8. Change Swing/Humanize and a lane Volume/Pan/Mute/Solo control.
9. Switch between 16/32/64-step lengths or pages.

Expected: the groove can be authored and edited entirely in JUCE.

### 4. Piano Roll and native instrument

1. Open Piano Roll on a MIDI Pattern.
2. Add several notes.
3. Move, resize and delete notes.
4. Change note velocity/length and grid.
5. Exercise root/scale guidance and keyboard preview.
6. Select at least one native FLOW instrument and modify an exposed instrument parameter.

Expected: a melodic part can be programmed and heard without external plugins.

### 5. External plugin instrument and effect racks

1. Scan plugins.
2. Assign a known-good VST3 instrument to a Track when an instrument fixture/plugin is available.
3. Play a Pattern through the external instrument and confirm note-on/note-off behavior.
4. Select **Track Rack**, add a scanned effect, then add a FLOW built-in effect.
5. Exercise **Enable/Disable**, **Bypass**, wet/dry and a native parameter.
6. Move inserts Up/Down and confirm the visible order changes.
7. Open the external insert editor, change a parameter, close it and reopen it; confirm state is restored.
8. Repeat rack management on **Master Rack**.
9. Create/select a Bus in the mixer, target **Bus: <name>** in the rack selector and repeat add/enable/bypass/wet/reorder/remove on the Bus rack.
10. Save, close and reopen the project. Confirm Track/Bus/Master rack order and plugin state persist.

Expected: all three mixer rack scopes are manageable from the production UI and external editor state belongs to the persistent project insert.

### 6. Recording and takes

1. Select/connect an input device.
2. Arm the intended recording track and enable input monitoring if desired.
3. Record a short microphone/audio take.
4. Stop recording and confirm the take appears in the project.
5. Record a second take.
6. Use Take - / Take + to switch the active non-destructive take.
7. Disconnect or deselect the input and confirm attempting another recording surfaces a useful error instead of crashing.

Expected: basic vocal/audio capture and comp selection work from the installed app.

### 7. Mixer, buses, sends and automation

1. Change Track volume/pan/mute/solo and confirm meters respond during playback.
2. Create or select a Bus.
3. Route a Track to a Bus.
4. Add a send, exercise enable/gain and pre/post-fader state.
5. Change Bus controls and confirm the route remains audible.
6. Write Track automation at the playhead.
7. Write Bus automation.
8. Write Master automation.
9. Play through the automated region and confirm the intended control changes are reflected.

Expected: a small production can be routed and automated without X11.

### 8. Save, reopen and recovery-sensitive state

1. Save the project as a .flow file.
2. Close the application normally.
3. Reopen the saved project.
4. Confirm imported assets, Pattern edits, MIDI, takes, routing, automation and plugin racks are still present.
5. Confirm a normal clean exit does not present a stale crash recovery state on the next launch.

Expected: the portable project round-trip is complete and clean exit behavior is sane.

### 9. Export

1. Export the master mix to WAV.
2. Play the resulting file outside FLOWDAW and confirm it contains the expected production.
3. Export stems.
4. Confirm per-track stem files are created and non-empty where the source track is audible.
5. For a project using a real external effect, confirm the exported result audibly includes the plugin processing.

Expected: realtime and offline/export paths agree closely enough for the same project to be delivered.

### 10. Phase 10 production-workflow closure

1. In Arrangement, select multiple audio/Pattern blocks with Ctrl/Cmd-click and confirm selected blocks remain visually identifiable.
2. Delete a multi-selection, then Undo/Redo and confirm the grouped edit behaves as one transaction.
3. Scroll the Arrangement horizontally, zoom with Ctrl/Cmd+wheel, drag a block with 1/16 snapping, then Alt-drag for free placement.
4. In the Mixer, switch the active target between a Track, a Bus and Master; confirm the header, supported controls and meter follow the same target.
5. On a Track target, change Output between Master and a Bus, create/update/remove a send, and exercise send gain plus pre/post mode.
6. Confirm changing Mixer Track/Bus/Master target also points the Plugin Rack at the corresponding rack, and changing the Rack target synchronizes the Mixer.
7. In the Sample Browser, search a folder, switch between Recent and Favorites, navigate with Up/Down and Home/End, preview with Space, stop with Escape and import with Enter.
8. Enable Auto Preview and confirm changing Browser selection auditions the newly selected WAV without blocking the UI.
9. In Piano Roll, Ctrl/Cmd-click multiple notes, use Ctrl/Cmd+A, grouped nudge/velocity/length edits and grouped Delete; confirm each grouped action is one Undo/Redo transaction.
10. In Step Sequencer, use arrows to navigate, Space to toggle, Delete to clear and Ctrl/Cmd+D to copy the selected step to the next step.
11. Scan plugins, search by name, filter All/Instruments/Effects and confirm instrument assignment, FX insertion, rack insertion and editor preview resolve the selected filtered plugin correctly.
12. Save/reopen the project and confirm all persisted musical/routing/plugin state remains intact as project format v11.

Expected: Phase 10 workflows are reachable from the production JUCE UI, remain Undo/Redo coherent, and do not require developer-only tooling.

### 11. Phase 11.5 musical-editor closure

1. Open Piano Roll on a MIDI Pattern. Confirm primary note selection, grouped selection and keyboard focus are visually distinct.
2. Change MIDI grid 1/8 → 1/16 → 1/32, root/scale and instrument context; add, move, resize, nudge and delete notes, then exercise Undo/Redo.
3. Open Step Sequencer. Confirm selected lane, focused step, enabled steps and beat/bar hierarchy are distinguishable across 16/32/64-step lengths/pages.
4. Edit step Velocity, Probability and Microtiming; change lane Volume/Pan/Mute/Solo and pattern Swing/Humanize; exercise keyboard navigation/toggle/clear/duplicate and Undo/Redo.
5. Open Sampler. Confirm selected pad/slice and transient audition feedback are visually different; navigate banks and trigger pads from mouse and mapped keyboard.
6. Rename a pad, change Gain/Pan/Choke, exercise Equal/Auto/Beat/Bar chop and Match BPM, then record REC CHOPS and apply Quantize/Humanize/Reset Feel.
7. Open Automation. Switch Track volume/pan, Bus volume/pan and Master volume targets; confirm target/route identity, parameter range, playhead/tick position and curve points remain readable.
8. Write multiple automation points at different playhead positions, write twice at one tick to exercise deterministic normalization, clear the lane, then Undo/Redo.
9. Save and reopen the .flow project. Confirm MIDI notes/instrument state, sequencer state, sample slices/chop events and AutomationLane state persist.
10. Play through the edited musical regions and confirm no editor requires the legacy X11 shell or a developer-only workflow.

Expected: Piano Roll, Sequencer, Sampler/REC CHOPS and Automation share one coherent FLOWDAW editing language while preserving their existing musical semantics, Undo/Redo boundaries and .flow v11 state.


### 12. Phase 11.6 adaptive-layout and accessibility closure

1. Launch at the default 1440×1040 logical size, then resize to 1280×800 and the supported 1180×720 minimum.
2. Confirm shell/project/transport/workspace controls remain inside the window and no primary control is clipped outside its parent.
3. Confirm the Sample Browser remains bounded while the active creative editor retains priority and stays usable.
4. At constrained height, confirm Mixer and Audio I/O collapse rather than forcing invalid editor geometry; use their visible restore controls after increasing available height.
5. At a larger window, show Mixer and Audio I/O and confirm Mixer height and utility height stay bounded instead of consuming unbounded creative space.
6. Open Automation and confirm Automation, Bus Mixer and Assist regions reflow from the current logical width rather than using fixed columns.
7. Tab and Shift+Tab through each workspace. Confirm visible/enabled controls receive focus in workflow order and hidden/collapsed controls are skipped.
8. In Piano Roll, Sequencer, Sampler and Automation, confirm keyboard focus is visible with an outline/shape cue and not conveyed by colour alone.
9. Type in plugin search and pad-name text fields. Confirm project/workspace shortcuts do not fire while the text field owns focus.
10. Open/close a plugin editor and switch workspaces; confirm useful FLOWDAW focus is restored where practical.
11. Inspect concise tooltips/help and accessible titles for ambiguous plugin, rack, Mixer, automation and panel controls.
12. Repeat the resize/focus/readability smoke at representative 100%, 125%, 150% and 200% native OS display scaling where the platform supports it. Record any platform-specific issue separately; automated CI does not substitute for physical display or assistive-technology inspection.
13. Save/reopen the project and confirm resizing/panel visibility/focus work did not change persisted musical state or the .flow v11 project contract.

Expected: resizing and keyboard/accessibility behavior improve the desktop workflow without changing musical semantics, realtime ownership or project persistence.

### 13. Phase 11.7 export / settings / diagnostics / recovery closure

1. Open **Export…** and confirm Master Mix and Track Stems are presented as the two delivery modes from one FLOWDAW export entry point.
2. Cancel destination selection for both modes and confirm no file/project mutation is reported; then complete a Master Mix export and a Track Stems export and verify explicit render/completion feedback.
3. Attempt an invalid/unwritable export destination where reproducible and confirm an actionable failure is shown without crashing or changing the project.
4. Open **Settings / Audio**, change a supported device/sample-rate/buffer setting, close the surface and confirm the creative workspace remains usable.
5. Exercise an unavailable/invalid audio-device case where reproducible and confirm FLOWDAW directs the user back to Settings / Audio and remains recoverable.
6. Open **Plugin Maintenance**. Run a plugin scan and confirm scan/validation/quarantine diagnostics are presented there while creative search/filter/selection/rack workflows remain in the main production workspace.
7. Exercise at least one failed/unavailable plugin case where reproducible and confirm repeated failure/quarantine behavior remains explicit rather than silently retrying in realtime.
8. Relaunch from each available startup state: fresh project, normal last-session restore and dirty autosave recovery. Confirm the startup identity/message distinguishes them.
9. Dismiss/cancel Open, Import WAV and Save dialogs and confirm the current project stays usable; exercise a failing Open/Import/Save path where reproducible and confirm contextual recovery guidance.
10. With no scanned plugin match and with no selected/imported sample, confirm the empty state points to a valid next action instead of blocking unrelated work.
11. Tab through visible Settings / Plugin Maintenance controls and resize to constrained supported geometry; confirm hidden utility controls do not compete with the creative surface.
12. Save/reopen the project and confirm Phase 11.7 settings/diagnostic/recovery presentation did not add device/UI/quarantine state to the portable `.flow` v11 project.

Expected: delivery, configuration, plugin maintenance and recovery/error workflows are understandable and recoverable from the production JUCE app while DSP, realtime ownership, PluginSafety, SessionRecovery and portable project semantics remain unchanged.


### 14. Phase 11.8 installed-app cross-workflow closure

Run this pass from the **installed/package artifact**, not a build-tree executable. Use the same artifact for the complete pass and record platform, package type and commit.

1. Launch FLOWDAW from the installed application and confirm the opening shell presents project identity, transport and workspace navigation as the dominant hierarchy; Settings / Audio and Plugin Maintenance must remain secondary surfaces rather than persistent dashboard panels.
2. At 1440×1040, move through Arrangement → Piano Roll → Sequencer → Sampler → Automation → Arrangement. Confirm each creative surface keeps the shared FLOWDAW dark music-production language, visible active-workspace state and readable focus without generic card/dashboard treatment.
3. Resize to 1280×800 and 1180×720. Keep the Sample Browser visible and request Mixer plus Settings / Audio. Confirm the creative editor retains usable geometry and secondary panels collapse/bound themselves rather than overlapping the editor.
4. In Arrangement, import a WAV from the Sample Browser, select/move an Arrangement block and Undo/Redo. Confirm Browser context, Arrangement selection and transient status remain visually distinguishable.
5. Switch to Mixer. Select Track → Bus → Master and confirm target identity, meter and rack target stay synchronized. Exercise a Track output route and send, then Undo/Redo where applicable.
6. In the plugin workflow, search/filter plugins and select a creative rack target. Then open Plugin Maintenance, confirm scan/quarantine diagnostics replace the utility surface rather than the creative editor, close it and verify keyboard focus/workspace interaction remains usable.
7. Visit Piano Roll, Sequencer, Sampler and Automation with an editable target. Make one reversible edit in each surface and confirm selection/focus/active state is visually distinct before Undo/Redo.
8. Open Settings / Audio, inspect the current device/sample-rate/buffer controls, close it and confirm no musical/project state changed. Reopen Plugin Maintenance and confirm the two maintenance surfaces are mutually exclusive.
9. Exercise Master Mix and Track Stems export from the production export controls. Cancel one chooser and complete the other; confirm cancellation preserves the project and completion/failure feedback is explicit.
10. Save the project, close FLOWDAW normally, relaunch the same installed application and reopen/restore the project. Confirm musical edits, routing, automation, samples and plugin rack state persist while panel visibility/device/diagnostic presentation remains machine-local.
11. Confirm a clean relaunch does not present stale crash recovery. Where a dirty autosave fixture is intentionally available, confirm recovered-autosave identity is explicit and Save/New/Open remain valid recovery actions.
12. Repeat the layout/workspace smoke on the platform's packaged artifact: Linux package, Windows package and macOS DMG/app as available in the required CI/release matrix. Platform rendering differences are acceptable only when hierarchy, bounds, focus visibility and workflow reachability remain equivalent.

Expected: the installed FLOWDAW application behaves as one coherent DAW across create/import → arrange → edit → mix → export/recover. Phase 11 presentation must preserve FLOWDAW's distinctive dense music-software identity, keep maintenance/configuration secondary to creative work, and leave DSP, realtime ownership and portable .flow v11 semantics unchanged.


## Failure conditions

Mark the run **FAIL** if any required flow needs X11, developer scripts, project-file hand editing, plugin creation/destruction from the audio callback, or if a normal user action causes a crash/hang. Also fail on silent loss of plugin state, rack order, recording takes, routing, automation or imported sample references after save/reopen.

## Automated evidence that complements this manual path

The manual run does not replace CI. Keep the existing automated coverage green for:

- serialization/migration and safe saves;
- Smart Sampling and Match BPM;
- recording;
- plugin state round-trip;
- realtime Track/Bus/Master graph and PDC;
- real VST3 host processing;
- real AU effect/instrument processing on macOS;
- external instrument MIDI note-on/note-off;
- realtime/offline parity;
- mix/stem export;
- install/package smoke tests;
- Phase 10 repeated graph-publication/render stress coverage;
- bounded preview-command saturation/recovery coverage.

The manual path verifies that those capabilities are actually reachable as one coherent product workflow.
