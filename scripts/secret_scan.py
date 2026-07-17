#!/usr/bin/env python3
"""Fail closed on publication secrets, local state, build outputs, and large artifacts.

The scanner intentionally checks the physical candidate tree rather than only
tracked Git paths. That makes a pre-init candidate and a post-init repository
obey the same public-release boundary.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

TEXT_SUFFIXES = {
    '', '.c', '.cc', '.cpp', '.h', '.hpp', '.ini', '.md', '.py', '.txt', '.yml', '.yaml',
    '.json', '.csv', '.html', '.xml', '.plist', '.dart', '.gradle', '.kts', '.sh', '.svg',
    '.properties', '.xcconfig', '.pbxproj', '.m', '.mm', '.swift', '.java', '.kt', '.cmake',
}
FORBIDDEN_SUFFIXES = {
    '.apk', '.aab', '.bin', '.elf', '.map', '.o', '.a', '.so', '.pem', '.key', '.p12',
    '.jks', '.zip', '.7z', '.tar', '.gz', '.ipa', '.mobileprovision',
}
FORBIDDEN_NAMES = {
    '.env', 'local.properties', 'wifi_credentials.h', 'credentials.h', 'secrets.h',
    'config.local.h', 'google-services.json', 'GoogleService-Info.plist',
    'id_rsa', 'id_ed25519', '.flutter-plugins', '.flutter-plugins-dependencies',
}
FORBIDDEN_DIRS = {
    '.pio', '.dart_tool', '.gradle', '.kotlin', '.idea', '.vscode', '.vs', 'build', 'dist',
    'release', 'coverage', '__pycache__', 'ephemeral', '.pub-cache', '.pub',
}
PATTERNS: list[tuple[str, re.Pattern[str]]] = [
    ('private key', re.compile(r'-----BEGIN (?:RSA |EC |OPENSSH |DSA )?PRIVATE KEY-----')),
    ('GitHub token', re.compile(r'\b(?:gh[opusr]_[A-Za-z0-9_]{20,}|github_pat_[A-Za-z0-9_]{20,})\b')),
    ('AWS access key', re.compile(r'\bAKIA[0-9A-Z]{16}\b')),
    ('OpenAI-style key', re.compile(r'\bsk-[A-Za-z0-9_-]{16,}\b')),
    ('private LAN literal', re.compile(r'\b(?:10\.\d{1,3}\.\d{1,3}\.\d{1,3}|192\.168\.\d{1,3}\.\d{1,3}|172\.(?:1[6-9]|2\d|3[01])\.\d{1,3}\.\d{1,3}|169\.254\.\d{1,3}\.\d{1,3})\b')),
    ('local absolute path', re.compile(r'/(?:home|Users|mnt)/[^\s`"\']+')),
    ('Windows user path', re.compile(r'(?i)\b[A-Z]:\\Users\\[^\\\s]+')),
    ('assigned secret', re.compile(r'''(?ix)\b(api[_-]?key|access[_-]?token|auth[_-]?token|secret|password|passwd|pwd)\b\s*[:=]\s*["'](?!YOUR_|EXAMPLE|REPLACE|CHANGEME|REDACTED|\[REDACTED\]|<YOUR_)([A-Za-z0-9+/=_!@#$%^&*.-]{8,})["']''')),
]
# Generic documentation/test examples, not device addresses. Every exception is
# deliberately exact so an actual LAN address still fails the scan.
ALLOWED_EXACT_LAN_LITERALS = {'192.168.1.50', '127.0.0.1', '10.0.0.2'}


def candidate_files(root: Path) -> list[Path]:
    return sorted(path for path in root.rglob('*') if path.is_file() and '.git' not in path.relative_to(root).parts)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', default='.')
    args = parser.parse_args()
    root = Path(args.root).resolve()
    errors: list[str] = []
    checked = candidate_files(root)
    for path in checked:
        rel = path.relative_to(root)
        if path.name in FORBIDDEN_NAMES:
            errors.append(f'{rel}: forbidden local/config file')
        if any(part in FORBIDDEN_DIRS for part in rel.parts):
            errors.append(f'{rel}: forbidden generated/release directory')
        if path.suffix.lower() in FORBIDDEN_SUFFIXES:
            errors.append(f'{rel}: forbidden binary/archive/key artifact')
        if path.stat().st_size > 5 * 1024 * 1024:
            errors.append(f'{rel}: file exceeds 5 MiB publication limit')
        if path.suffix.lower() not in TEXT_SUFFIXES or path.stat().st_size > 2_000_000:
            continue
        try:
            text = path.read_text(encoding='utf-8')
        except (UnicodeDecodeError, OSError):
            continue
        for number, line in enumerate(text.splitlines(), 1):
            for label, pattern in PATTERNS:
                if label == 'private LAN literal' and any(value in line for value in ALLOWED_EXACT_LAN_LITERALS):
                    # The line may contain one generic placeholder. Check any
                    # remaining LAN address before accepting it.
                    values = pattern.findall(line)
                    if values and all(value in ALLOWED_EXACT_LAN_LITERALS for value in values):
                        continue
                if pattern.search(line):
                    errors.append(f'{rel}:{number}: {label}')
    if errors:
        print('Secret/publication scan: FAIL', file=sys.stderr)
        print('\n'.join(sorted(set(errors))), file=sys.stderr)
        return 1
    print(f'Secret/publication scan: PASS ({len(checked)} files checked)')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
