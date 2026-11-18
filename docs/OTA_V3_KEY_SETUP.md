# OTA V3 signing-key setup

OTA V3 intentionally fails closed until a real signing public key is embedded in firmware.

## Key model

- ECDSA P-256 with SHA-256.
- Firmware contains only the PEM public key in include/Ava_OTA_V3_PublicKey.h.
- GitHub Actions stores the PEM private key in repository secret AVA_OTA_SIGNING_PRIVATE_KEY.
- The detached signature is Base64-encoded DER ECDSA, stored as ota/manifest.sig.
- The signature covers the exact bytes of ota/manifest.json. Do not reformat a signed manifest.

## Generate and configure

Run from the repository root in PowerShell:

    .\tools\Setup-OTA-V3-Key.ps1 -SetGitHubSecret

Requirements:
- OpenSSL available directly or through Git for Windows.
- GitHub CLI installed and authenticated with permission to manage repository Actions secrets.

The script creates the private key outside the repository under %USERPROFILE%\AVA-OTA-V3-Key, writes the public-key header into include, and uploads the private key to GitHub Actions.

## Important

- Never commit, upload as an artifact, or paste the private key into chat.
- Keep an offline backup of the private key. Losing it means future manifests cannot be signed with the public key already embedded in deployed firmware.
- The public key is an identity anchor. Rotating it requires a deliberate firmware key-rotation plan.
- The checked-in public-key header is currently a placeholder. Until the real public key is generated and deployed in firmware, OTA V3 rejects all manifests.
