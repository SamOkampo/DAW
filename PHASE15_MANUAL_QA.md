# Phase 15 Physical Installed-App Validation

**State: HUMAN / HARDWARE GATE — evidence required**

Run the same release-candidate commit and record artifact checksum for every pass.

## Windows

- Install/extract the RC on a clean or representative Windows system.
- Launch the production FLOWDAW executable.
- Validate built-in FLOW Core playback with no third-party plugins.
- Validate one representative ASIO/WASAPI device path available to the tester.
- Record and play back a short audio take.
- Validate one known-good VST3 instrument and one VST3 effect.
- Save/reopen a `.flow` project.
- Export master WAV and stems.
- Exercise 100/125/150/200% display scaling where practical.
- Record any SmartScreen/signing behavior.

## macOS

- Mount/copy the exact DMG/app candidate.
- Validate built-in playback, recording and save/reopen.
- Validate one known-good VST3 and one AU instrument/effect path.
- Export master/stems.
- Exercise representative display scaling.
- Record Gatekeeper/notarization behavior.

## Linux

- Extract the exact TGZ candidate.
- Validate launch, built-in playback, recording where hardware is available and a known-good VST3.
- Save/reopen and export.
- Confirm FLOW Core resolves after moving the extracted package to a different absolute path.

## Pass rule

A platform passes only with recorded human evidence from the packaged artifact. CI fixture tests complement this gate but cannot mark it PASS by themselves.
