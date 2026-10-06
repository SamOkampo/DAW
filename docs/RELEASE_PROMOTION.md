# FLOWDAW RC → 1.0 Promotion Procedure

The release candidate is `1.0.0-rc.1`. Promotion to 1.0 is a gate, not a feature-development phase.

## Required evidence

1. The exact candidate commit passes the full FLOWDAW CI matrix.
2. Linux TGZ, Windows ZIP and macOS DMG are produced from that commit.
3. Each artifact has a release manifest and SHA-256 checksum.
4. The owner-selected FLOWDAW distribution terms and applicable JUCE 9 licensing path are recorded.
5. Required third-party notices are reviewed.
6. Windows production signing is verified.
7. macOS signing/notarization is verified.
8. `PHASE15_MANUAL_QA.md` has platform evidence for the required installed-app checks.
9. No open release-blocking issue remains.

## Promotion change

Only after the evidence above:

- set the human-facing release version from `1.0.0-rc.1` to `1.0.0`;
- update changelog/release notes from candidate to final;
- generate fresh packages and checksums from the final commit;
- sign/notarize the final artifacts rather than reusing signatures from an older candidate;
- record final artifact hashes and commit;
- mark Phase 15 CLOSED/PASS.

## Rollback rule

If a final artifact, signing result or physical validation fails, do not rewrite the same published artifact in place. Correct the source/build/release configuration, create a new candidate and repeat the gate.
