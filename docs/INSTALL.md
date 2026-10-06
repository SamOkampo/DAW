# Installing FLOWDAW 1.0.0-rc.1

FLOWDAW's production application is the JUCE desktop build. The legacy X11 shell is not the production product.

## Windows

1. Obtain the FLOWDAW Windows release ZIP from the matching release/CI artifact.
2. Verify the published SHA-256 checksum before extracting.
3. Extract the complete archive to a writable application folder.
4. Launch `bin/FLOWDAW.exe`.
5. Keep `share/FLOWDAW/content/flow-core` beside the installed layout so bundled content can be discovered.
6. Configure your audio device in Settings / Audio before recording.

Unsigned RC artifacts may trigger operating-system reputation warnings. Do not describe an unsigned RC as a signed production release.

## macOS

1. Obtain the matching FLOWDAW DMG and verify its SHA-256 checksum.
2. Mount the DMG and copy `FLOWDAW.app` to Applications.
3. Launch FLOWDAW and configure Settings / Audio.
4. FLOW Core must remain inside the application resources.

A production macOS release should be Developer ID signed and notarized. An RC that has not passed that gate must be labelled accordingly.

## Linux

1. Obtain the matching TGZ and verify its SHA-256 checksum.
2. Extract the archive without flattening its directory structure.
3. Launch `bin/FLOWDAW`.
4. Configure the audio device and plugin roots as needed.

## First launch

Use a FLOW Core starter project to confirm native audio works before scanning third-party plugins. Save a test `.flow` project, reopen it, and complete one master WAV export before starting important work.

See `QUICK_START.md`, `TROUBLESHOOTING.md`, and `PRODUCTION_GOLDEN_PATH.md`.
