#!/usr/bin/env python3
"""Ensure the OTA signing key matches the public key embedded in firmware."""
import pathlib
import re
import sys

def normalize_pem(data: str) -> str:
    return "".join(data.split())

def main() -> int:
    if len(sys.argv) not in (3, 4):
        print(
            "usage: verify_ota_v3_public_key.py derived-public.pem public-key-header.h [CONFIGURED_MACRO]",
            file=sys.stderr,
        )
        return 2
    derived = pathlib.Path(sys.argv[1]).read_text(encoding="ascii")
    header = pathlib.Path(sys.argv[2]).read_text(encoding="utf-8")
    configured_macro = (
        sys.argv[3] if len(sys.argv) == 4
        else "AVA_OTA_V3_PUBLIC_KEY_CONFIGURED"
    )
    match = re.search(r"-----BEGIN PUBLIC KEY-----.*?-----END PUBLIC KEY-----",
                      header, flags=re.DOTALL)
    if not match or "REPLACE_WITH" in match.group(0):
        print("OTA V3 public key is still a placeholder.", file=sys.stderr)
        return 1
    if normalize_pem(derived) != normalize_pem(match.group(0)):
        print("Signing key does not match the public key embedded in firmware.", file=sys.stderr)
        return 1
    if f"#define {configured_macro} 1" not in header:
        print(f"Public key header is not marked configured ({configured_macro}).", file=sys.stderr)
        return 1
    print("OTA V3 signing key matches the firmware verification key.")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
