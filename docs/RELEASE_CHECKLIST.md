# FLOWDAW Release Checklist

Target candidate: **1.0.0-rc.1**

## Automated

- [ ] Phase 15 PR CI is fully green.
- [ ] Core/legacy/JUCE Linux/Windows/macOS matrices pass.
- [ ] Package creation succeeds on all required platforms.
- [ ] Release manifest + SHA-256 files are generated from the exact candidate commit.
- [ ] FLOW Core package integrity checks pass.
- [ ] Mastering golden path remains green.
- [ ] README/changelog/release notes match the candidate version.

## Owner / legal

- [ ] FLOWDAW software/content distribution licence or EULA is explicitly selected.
- [ ] The selected JUCE 9 licensing path is documented and satisfied for distribution.
- [ ] Required third-party notices are reviewed.

## Protected release credentials

- [ ] Windows production binary/package is signed and signature verified.
- [ ] macOS application/DMG is signed/notarized/stapled and verified.

## Human hardware

- [ ] Windows packaged-artifact QA evidence recorded.
- [ ] macOS packaged-artifact QA evidence recorded.
- [ ] Linux packaged-artifact QA evidence recorded.
- [ ] Representative audio recording/playback tested.
- [ ] Representative third-party VST3/AU tests recorded.
- [ ] Display-scaling/accessibility smoke recorded.

## Promotion

Promote RC to 1.0 only when every required checkbox above is evidenced. Do not convert an unavailable external gate into a synthetic PASS.
