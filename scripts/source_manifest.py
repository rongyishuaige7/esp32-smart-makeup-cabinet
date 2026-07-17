#!/usr/bin/env python3
"""Compute a credential-safe source manifest from an explicit allowlist.

It never performs a broad recursive read of the original source. The caller
must explicitly provide an authorized, read-only source directory.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ALLOWLIST = ROOT / 'docs/source-allowlist.txt'
FORBIDDEN = {
    'src/config.h',
    'src/config.local.h',
    'src/wifi_credentials.h',
    'src/credentials.h',
    'src/secrets.h',
    'smart_cabinet_app/android/local.properties',
}


def safe_paths() -> list[str]:
    rows: list[str] = []
    for raw in ALLOWLIST.read_text(encoding='utf-8').splitlines():
        value = raw.strip()
        if not value or value.startswith('#'):
            continue
        candidate = Path(value)
        if candidate.is_absolute() or '..' in candidate.parts:
            raise ValueError(f'unsafe allowlist path: {value}')
        rows.append(value)
    if rows != sorted(rows):
        raise ValueError('allowlist paths must be sorted')
    if len(rows) != len(set(rows)):
        raise ValueError('allowlist paths must be unique')
    overlap = FORBIDDEN.intersection(rows)
    if overlap:
        raise ValueError(f'credential/local file in allowlist: {sorted(overlap)}')
    return rows


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', required=True, help='authorized read-only original source directory')
    args = parser.parse_args()
    source = Path(args.source).resolve()
    if not source.is_dir():
        raise SystemExit(f'not a directory: {source}')
    digest = hashlib.sha256()
    paths = safe_paths()
    for rel in paths:
        path = source / rel
        if not path.is_file():
            raise SystemExit(f'missing allowlisted source file: {rel}')
        digest.update(rel.encode('utf-8'))
        digest.update(b'\0')
        # Reading is restricted to the reviewed allowlist above.
        digest.update(path.read_bytes())
        digest.update(b'\0')
    print(f'files={len(paths)}')
    print(f'sha256={digest.hexdigest()}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
