# Phase 15 Status — Release Readiness / Distribution

**Status: IN PROGRESS**

## Baseline

- Phase 14 is complete and audited on `main`.
- Authoritative technical baseline: commit `5135d6ee32773c6c20d2b8f0272da91e37a92619`.
- Production desktop remains JUCE 9.0.2.
- Portable project format remains `.flow` v11.
- Release candidate is now deliberately versioned as 1.0.0-rc.1; portable project format remains v11.
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

- **15.3 Release artifacts / versioning / integrity — IMPLEMENTED / CI PENDING.**
  - Candidate version: `1.0.0-rc.1`.
  - CMake/app packaging metadata, changelog and RC release notes are synchronized.
  - `tools/release_manifest.py` generates deterministic SHA-256 manifests and checksum files.
  - Linux/Windows/macOS main-branch CI uploads packaged RC artifacts plus matching manifest/checksum metadata.

- **15.4 Signing / notarization — TECHNICAL POLICY DONE / CREDENTIAL GATE PENDING.**
  - `docs/SIGNING.md` defines the protected production gate and forbids committing credentials.
  - Windows/macOS signing/notarization cannot truthfully be marked PASS until valid owner credentials execute against the exact release artifact.

- **15.5 Physical installed-app validation — TEST PLAN DONE / HUMAN EVIDENCE PENDING.**
  - `PHASE15_MANUAL_QA.md` defines Windows/macOS/Linux package-level evidence for audio, recording, plugins, save/reopen, export and display scaling.
  - CI cannot self-certify this hardware gate.

- **15.6 Public onboarding / support / privacy docs — DONE on branch / CI PENDING.**
  - Added install, quick-start, troubleshooting, known-limitations, privacy, support and signing documentation.
  - Current privacy statement explicitly records that this source baseline has no FLOWDAW account, analytics SDK, advertising SDK or telemetry pipeline.

- **15.7 Release candidate / beta gate — RC PREP DONE / INTEGRATION + EXTERNAL EVIDENCE PENDING.**
  - RC identity, changelog, release notes, integrity tooling and cross-platform artifact plumbing are implemented.
  - Promotion still requires full green CI plus the explicit legal/signing/manual checklist.

- **15.8 1.0 release closure — PENDING.**
  - Final release artifacts/docs/checksums plus completed legal, signing/notarization and manual gates.

## What “finished” means

FLOWDAW is technically feature-complete through Phase 14. Phase 15 is complete only when the release can be distributed truthfully and supportably; CI alone cannot satisfy legal choices, private signing credentials or physical-device validation.
