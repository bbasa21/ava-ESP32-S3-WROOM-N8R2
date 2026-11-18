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

Commit and push the generated public-key header to the feature branch (never add the private-key file):

    git add .\include\Ava_OTA_V3_PublicKey.h
    git commit -m "security(ota): configure V3 public verification key"
    git push origin feature/ota-v3

## Important

- Never commit, upload as an artifact, or paste the private key into chat.
- Keep an offline backup of the private key. Losing it means future manifests cannot be signed with the public key already embedded in deployed firmware.
- The public key is an identity anchor. Rotating it requires a deliberate firmware key-rotation plan.
- The checked-in public-key header is currently a placeholder. Until the real public key is generated and deployed in firmware, OTA V3 rejects all manifests.


## Isolated hardware test channel (does not touch production `main`)

After the generated public-key header has been committed to `feature/ota-v3` and the Actions secret is configured:

1. Build and USB-flash the test-channel firmware from the project root:

       pio run -e ava_ota_v3_test --target upload

   This build reads only `ota/v3-test-manifest.json` and accepts only release tags prefixed with `ota-v3-test-`. The normal `ava` environment remains pointed at the production manifest on `main`.

2. Trigger a signed staging release by changing the dedicated trigger file and pushing the feature branch:

       Set-Content .\ota\test-release.trigger ("staging-" + (Get-Date -Format "yyyyMMdd-HHmmss"))
       git add .\ota\test-release.trigger
       git commit -m "test(ota): trigger signed V3 staging release"
       git push origin feature/ota-v3

3. Inspect the Actions run named **AVA OTA V3 Test Release**. It builds a staging firmware, signs its manifest and publishes the test manifest only on `feature/ota-v3`.

4. On COM3, send `ota check`, then `ota status`. After the signed test build appears as an update, send `ota install`.

The staging workflow uses build numbers in the 20000+ range so its test releases are newer than the current development build number. Repeating the trigger creates the next staging update. Never copy a private key into the repository or into chat.
