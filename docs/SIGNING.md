# FLOWDAW Release Signing / Notarization

Signing material is an external release secret and must never be committed to this repository.

## Windows production gate

A production Windows artifact should be Authenticode-signed with the product owner's valid code-signing certificate and verified after signing. The release record should capture the certificate identity, timestamping result and SHA-256 of the final signed artifact.

## macOS production gate

A production macOS application should be signed with an appropriate Developer ID identity, packaged, submitted for Apple notarization, stapled where applicable and validated with Apple's tooling. The release record should capture the notarization result and SHA-256 of the final DMG.

## CI policy

Normal pull-request CI intentionally validates unsigned deterministic packages. Production signing belongs to a protected release environment using repository/environment secrets and should operate on the exact release candidate commit.

If signing credentials are unavailable, the release must remain labelled RC/unsigned rather than silently claiming the gate passed.
