# FLOWDAW Troubleshooting

## No audio

Open Settings / Audio and verify the selected output device, sample rate and buffer. Confirm the operating system has not assigned the device exclusively to another application.

## Recording does not start

Confirm an input device exists, select the intended recording track, arm it and verify the input is visible to FLOWDAW. FLOWDAW should surface an unavailable-input error rather than crash.

## Plugin is missing

Open Plugin Maintenance, confirm the plugin format is supported on the current platform, verify the scan root where applicable and rescan. VST3 is supported on Linux/Windows/macOS; AU hosting is macOS-only.

## Plugin is quarantined

FLOWDAW tracks repeated plugin failures. Review Plugin Maintenance or run:

`flowdaw-doctor clear-quarantine <plugin-id>`

Only clear quarantine after addressing the plugin/runtime problem.

## Project recovery

After an abnormal exit, FLOWDAW can offer an autosave recovery state. Save recovered work to a normal `.flow` path before continuing.

CLI helpers:

- `flowdaw-doctor status`
- `flowdaw-doctor recover recovered.flow`
- `flowdaw-doctor clear-recovery`

## FLOW Core content is missing

Use the complete installed/package layout. Do not move only the executable out of its package hierarchy. FLOW Core discovery is relative to the installed product layout.

## Export sounds different

Confirm the same Track/Bus/Master routing and plugin state is active. FLOWDAW has automated realtime/offline parity coverage, but third-party plugins can still have vendor-specific realtime/offline behavior.

## Reporting a reproducible problem

Record FLOWDAW version, platform/OS, audio device, sample rate/buffer, plugin name/version if relevant, exact steps, expected result, actual result and whether a fresh FLOW Core project reproduces it. Do not attach private projects or audio unless you intend to share them.
