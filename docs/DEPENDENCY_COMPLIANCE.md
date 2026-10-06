# FLOWDAW Dependency / Distribution Compliance Gate

This document is a release checklist, not legal advice.

## JUCE 9.0.2

FLOWDAW currently pins JUCE 9.0.2 for the production desktop.

As of the Phase 15 release-readiness audit, JUCE's official materials describe two broad ways to use JUCE 9:

1. use JUCE under the applicable JUCE 9 licence type/EULA; or
2. use JUCE under AGPLv3 and satisfy the obligations of that licence.

Official upstream references:

- https://juce.com/legal/juce-9-licence/
- https://juce.com/get-juce/

The upstream JUCE 9 EULA also defines Starter, Indie, Pro and Educational licence types with eligibility/usage conditions. FLOWDAW must not assume a tier from repository data.

## FLOWDAW owner decision required

Before public production distribution, the product owner must select and document one coherent distribution model.

### Route A — proprietary/commercial FLOWDAW

- Select a FLOWDAW proprietary software/content EULA or other distribution terms.
- Hold and maintain the JUCE licence required for the actual owner/team/use/distribution facts.
- Preserve required third-party notices.
- Keep signing/notarization credentials external to source control.

### Route B — AGPLv3-compatible FLOWDAW distribution

- Explicitly license the covered FLOWDAW source under terms compatible with the chosen AGPLv3 path.
- Satisfy the complete corresponding-source and distribution obligations.
- Preserve third-party notices.

Do not mix the two routes accidentally. The current repository intentionally does not add a top-level public licence merely to make CI green.

## First-party content

FLOW Core provenance is recorded by `PHASE12_7_AUDIT.md`, generated `PROVENANCE.txt` and `CONTENT_RIGHTS.txt`. Those files establish origin/integrity; the final public content-use grant must align with the product owner's selected distribution terms.

## Release gate

15.2 technical audit is ready when:

- this compliance inventory is present;
- `THIRD_PARTY_NOTICES.md` is present;
- the repository does not falsely claim a licence path has been chosen;
- the owner-selected FLOWDAW/JUCE path is recorded before 1.0 production promotion.
