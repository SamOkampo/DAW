# Phase 15 Status — Release Readiness / Distribution

**Status: IN PROGRESS**

## Baseline

- Phase 14 is complete and audited on `main`.
- Authoritative technical baseline: commit `5135d6ee32773c6c20d2b8f0272da91e37a92619`.
- Production desktop remains JUCE 9.0.2.
- Portable project format remains `.flow` v11.
- Application version remains 0.8.0 until the release-version gate deliberately changes it.
- Phase 15 must not add speculative DSP/product scope merely to create more phases.

## 15.1 — Release truth / metadata baseline

**State: DONE on this branch, pending PR/CI/merge.**

Acceptance:
- README no longer claims Phase 11 is still in progress.
- Roadmap records Phase 14 as the frozen technical baseline and defines the release-readiness sequence.
- Remaining owner/external gates are explicit rather than being silently treated as CI-complete.
- No realtime, DSP, serialization or project-format behavior changes.

## Remaining sequence

- **15.2 Licensing / dependency compliance — PENDING.**
  - FLOWDAW currently has no declared top-level product licence/EULA in the audited Phase 12.7 baseline.
  - Product owner must choose the intended source/distribution/commercial terms.
  - Applicable JUCE and dependency obligations must be satisfied before a public/commercial-ready claim.

- **15.3 Release artifacts / versioning / integrity — PENDING.**
  - Decide RC/application version.
  - Generate release notes/changelog plus artifact hashes/manifests and traceability.

- **15.4 Signing / notarization — PENDING / EXTERNAL CREDENTIALS.**
  - Windows signing and macOS signing/notarization where applicable.
  - Certificates/tokens remain outside the repository.

- **15.5 Physical installed-app validation — PENDING / HUMAN HARDWARE GATE.**
  - Run installed/package golden path on real systems.
  - Validate representative audio/MIDI devices, scaling and third-party plugin scans.

- **15.6 Public onboarding / support / privacy docs — PENDING.**
  - Installation, quick start, known limitations, recovery/support, plugin troubleshooting and privacy/telemetry statement.

- **15.7 Release candidate / beta gate — PENDING.**
  - Tagged RC, full CI green, required manual evidence and no release-blocking issue.

- **15.8 1.0 release closure — PENDING.**
  - Final release artifacts/docs/checksums plus completed legal, signing/notarization and manual gates.

## What “finished” means

FLOWDAW is technically feature-complete through Phase 14. Phase 15 is complete only when the release can be distributed truthfully and supportably; CI alone cannot satisfy legal choices, private signing credentials or physical-device validation.
