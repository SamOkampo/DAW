#!/usr/bin/env python3
"""Generate deterministic SHA-256 release metadata for FLOWDAW artifacts."""
from __future__ import annotations
import argparse, hashlib, json
from pathlib import Path

def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()

def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--version", required=True)
    p.add_argument("--commit", required=True)
    p.add_argument("--platform", required=True)
    p.add_argument("--output", default="release-manifest.json")
    p.add_argument("--checksums", default="SHA256SUMS")
    p.add_argument("artifacts", nargs="+")
    args = p.parse_args()

    artifacts = []
    for raw in sorted(args.artifacts):
        path = Path(raw)
        if not path.is_file():
            raise SystemExit(f"artifact not found: {path}")
        artifacts.append({
            "name": path.name,
            "bytes": path.stat().st_size,
            "sha256": sha256(path),
        })

    manifest = {
        "schema": 1,
        "product": "FLOWDAW",
        "version": args.version,
        "commit": args.commit,
        "platform": args.platform,
        "artifacts": artifacts,
    }
    Path(args.output).write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    Path(args.checksums).write_text(
        "".join(f"{item['sha256']}  {item['name']}\n" for item in artifacts),
        encoding="utf-8",
    )
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
