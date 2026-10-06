# FLOWDAW 1.0.0-rc.1 Known Limitations

- The release candidate is not the final 1.0 production declaration until licensing, signing/notarization and physical installed-app gates are complete.
- External VST3/AU behavior can vary by vendor; a green fixture matrix does not certify every third-party plugin.
- AU hosting is macOS-only.
- The legacy X11 shell remains a regression/bootstrap target, not the production UI.
- The mastering implementation targets ITU-R BS.1770-5 / EBU R128 behavior but FLOWDAW does not claim third-party ITU/EBU certification.
- CI display/layout coverage does not substitute for physical high-DPI, accessibility or multi-device inspection.
- Audio-device behavior can vary by driver and operating-system configuration.
- Unsigned/unnotarized RC artifacts may show OS trust/reputation warnings until the signing gate is completed.
