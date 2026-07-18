#!/usr/bin/env python3
"""Static publication contracts that require no physical ESP32 hardware."""
from __future__ import annotations

import argparse
import csv
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

REQUIRED = [
    '.github/workflows/validate.yml', '.gitattributes', '.gitignore', '.markdownlint-cli2.jsonc',
    'HARDWARE.md', 'LICENSE', 'README.md', 'SECURITY.md', 'THIRD_PARTY_NOTICES.md',
    'docs/GITHUB_METADATA.md', 'docs/HARDWARE_LAB_CARD.md', 'docs/PROJECT_STATUS.md',
    'docs/PROTOCOL.md', 'docs/SOURCE_PROVENANCE.md', 'docs/VERIFICATION.md',
    'docs/source-allowlist.txt', 'hardware/BOM.csv', 'hardware/wiring-diagram.svg',
    'firmware/platformio.ini', 'firmware/src/config.h', 'firmware/src/wifi_credentials.example.h',
    'scripts/check_repo.py', 'scripts/secret_scan.py', 'scripts/source_manifest.py', 'scripts/verify.sh',
    'tests/test_source_contracts.py', 'mobile/smart_cabinet_app/pubspec.yaml',
    'mobile/smart_cabinet_app/pubspec.lock', 'mobile/smart_cabinet_app/test/widget_test.dart',
    'mobile/smart_cabinet_app/android/gradlew', 'mobile/smart_cabinet_app/android/gradlew.bat',
    'mobile/smart_cabinet_app/android/gradle/wrapper/gradle-wrapper.jar',
    'mobile/smart_cabinet_app/android/gradle/wrapper/gradle-wrapper.properties',
]
FORBIDDEN_NAMES = {
    '.env', 'local.properties', 'wifi_credentials.h', 'credentials.h', 'secrets.h', 'config.local.h',
    'google-services.json', 'GoogleService-Info.plist', '.flutter-plugins', '.flutter-plugins-dependencies',
}
FORBIDDEN_DIRS = {
    '.pio', '.gradle', '.dart_tool', '.kotlin', '.idea', '.vscode', '.vs', 'build', 'dist', 'release',
    'coverage', 'ephemeral', '__pycache__', '.pub-cache', '.pub',
}
FORBIDDEN_SUFFIXES = {
    '.o', '.a', '.elf', '.bin', '.map', '.pyc', '.apk', '.aab', '.so', '.pem', '.key', '.zip', '.7z',
    '.tar', '.gz', '.ipa', '.mobileprovision',
}


def files(root: Path) -> list[Path]:
    return sorted(path for path in root.rglob('*') if path.is_file() and '.git' not in path.relative_to(root).parts)


def text(root: Path, rel: str) -> str:
    return (root / rel).read_text(encoding='utf-8')


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', default='.')
    args = parser.parse_args()
    root = Path(args.root).resolve()
    errors: list[str] = []
    for rel in REQUIRED:
        if not (root / rel).is_file():
            errors.append(f'missing required file: {rel}')
    for path in files(root):
        rel = path.relative_to(root)
        if path.name in FORBIDDEN_NAMES:
            errors.append(f'forbidden local/config file: {rel}')
        if any(part in FORBIDDEN_DIRS for part in rel.parts):
            errors.append(f'forbidden generated/release directory: {rel}')
        if path.suffix.lower() in FORBIDDEN_SUFFIXES:
            errors.append(f'forbidden binary/archive/key artifact: {rel}')
        if path.stat().st_size > 5 * 1024 * 1024:
            errors.append(f'file exceeds 5 MiB: {rel}')
    contracts = {
        'README.md': [
            '公开默认', 'HTTP `200` 或 `accepted:true` 只表示',
            '不是化妆品质量、真假、过期真实性',
        ],
        'firmware/platformio.ini': ['platform = espressif32@6.13.0', 'board = esp32dev'],
        'firmware/src/config.h': [
            '#define WIFI_SSID ""', '#define ENABLE_EXPERIMENTAL_ACTUATORS 0',
            '#define ENABLE_EXPERIMENTAL_LOCAL_CONTROL 0', '#if __has_include("config.local.h")',
        ],
        'firmware/src/http_server.cpp': [
            'document["publicDefaultInert"]', 'document["physicalPositionVerified"] = false',
            'server.on("/api/latch", HTTP_POST, handleLatch)', 'accepted\\":true',
            'raw_rfid_scan_not_available_over_public_http', 'experimental_local_control_disabled',
        ],
        'mobile/smart_cabinet_app/lib/services/cabinet_service.dart': [
            'normalizeLocalHttpUrl', "'/api/latch'", "result['accepted'] != true", 'isPrivateOrLoopbackHost',
        ],
        'mobile/smart_cabinet_app/lib/providers/cabinet_provider.dart': [
            'Future<void> testConnection()', '_service.getSensors()', '_service.getStatus()',
            '旧报告已清空',
        ],
        'docs/SOURCE_PROVENANCE.md': ['只读取该 allowlist', '不读取或哈希可能包含凭据/本机状态'],
        'docs/PROTOCOL.md': ['公开默认不开 Wi-Fi/HTTP', '`/api/latch`', '`physicalPositionVerified`'],
        'SECURITY.md': ['RFID UID 可复制', '禁止公网暴露'],
    }
    for rel, values in contracts.items():
        path = root / rel
        if not path.is_file():
            continue
        value = text(root, rel)
        for required in values:
            if required not in value:
                errors.append(f'fact contract missing in {rel}: {required}')
    try:
        ET.parse(root / 'hardware/wiring-diagram.svg')
    except (ET.ParseError, OSError) as exc:
        errors.append(f'invalid wiring SVG: {exc}')
    try:
        rows = list(csv.DictReader((root / 'hardware/BOM.csv').open(newline='', encoding='utf-8')))
        if len(rows) < 11:
            errors.append('BOM must contain at least 11 component rows')
    except (OSError, csv.Error) as exc:
        errors.append(f'invalid BOM.csv: {exc}')
    # When the candidate has been initialized, make the tracked set itself a
    # publication contract. A clean clone must contain every required file and
    # platform entrypoint; local ignore rules must not silently omit them.
    git_dir = root / '.git'
    if git_dir.is_dir():
        try:
            tracked = set(subprocess.check_output(
                ['git', '-C', str(root), 'ls-files'], text=True, stderr=subprocess.PIPE
            ).splitlines())
            must_track = REQUIRED + [
                'mobile/smart_cabinet_app/android/app/src/main/java/io/flutter/plugins/GeneratedPluginRegistrant.java',
                'mobile/smart_cabinet_app/ios/Runner/GeneratedPluginRegistrant.h',
                'mobile/smart_cabinet_app/ios/Runner/GeneratedPluginRegistrant.m',
            ]
            for rel in must_track:
                if rel not in tracked:
                    errors.append(f'required publish file is not tracked: {rel}')
        except (OSError, subprocess.CalledProcessError) as exc:
            errors.append(f'cannot inspect tracked publish files: {exc}')
    forbidden_claims = ['system online', 'current hardware verified', 'hardware re-verified: pass']
    for rel in ['README.md', 'docs/PROJECT_STATUS.md', 'docs/HARDWARE_LAB_CARD.md', 'docs/GITHUB_METADATA.md']:
        path = root / rel
        if not path.is_file():
            continue
        lowered = text(root, rel).lower()
        for claim in forbidden_claims:
            if claim in lowered:
                errors.append(f'unsupported claim in {rel}: {claim}')
    if errors:
        print('Repository check: FAIL', file=sys.stderr)
        for item in sorted(set(errors)):
            print(f'- {item}', file=sys.stderr)
        return 1
    print(f'Repository check: PASS ({len(files(root))} files checked)')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
