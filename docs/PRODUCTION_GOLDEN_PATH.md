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
- install/package smoke tests.

The manual path verifies that those capabilities are actually reachable as one coherent product workflow.
