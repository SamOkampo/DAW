# FLOWDAW Owner Release Decisions

The repository has completed the technical preparation around these decisions, but cannot make them on behalf of the product owner.

## Decision 1 — distribution model

Choose one coherent path before final 1.0 production promotion:

- proprietary/commercial FLOWDAW terms + the applicable JUCE 9 licence; or
- an AGPLv3-compatible open-source distribution path with its obligations.

Record the selected terms in the repository/release package.

## Decision 2 — signing identities

Provide protected Windows and macOS signing/notarization credentials through the release environment. Never commit private keys, certificate passwords, Apple credentials or tokens.

## Decision 3 — human validation evidence

Run the package-level tests in `PHASE15_MANUAL_QA.md` on representative real systems and record results.

These are release-owner gates, not missing DSP/code work.
