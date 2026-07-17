#!/usr/bin/env bash
# Public release gate. It intentionally builds firmware in a temporary copy;
# no build output is allowed to remain in the candidate repository.
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
PYCACHE="$(mktemp -d)"
PIO_WORK_DIR="$(mktemp -d)"
FLUTTER_DIR="$ROOT/mobile/smart_cabinet_app"
FLUTTER_GENERATED_PATHS=(
  "$FLUTTER_DIR/.dart_tool" "$FLUTTER_DIR/.flutter-plugins" "$FLUTTER_DIR/.flutter-plugins-dependencies"
  "$FLUTTER_DIR/.pub-cache" "$FLUTTER_DIR/.pub" "$FLUTTER_DIR/build" "$FLUTTER_DIR/coverage"
  "$FLUTTER_DIR/android/.gradle" "$FLUTTER_DIR/android/.kotlin" "$FLUTTER_DIR/android/local.properties"
  "$FLUTTER_DIR/ios/Flutter/Generated.xcconfig" "$FLUTTER_DIR/ios/Flutter/flutter_export_environment.sh"
  "$FLUTTER_DIR/ios/Flutter/ephemeral" "$FLUTTER_DIR/ios/Flutter/.last_build_id"
  "$FLUTTER_DIR/macos/Flutter/ephemeral" "$FLUTTER_DIR/linux/flutter/ephemeral" "$FLUTTER_DIR/windows/flutter/ephemeral"
)
cleanup() {
  rm -rf -- "$PYCACHE" "$PIO_WORK_DIR" "$ROOT/tests/__pycache__" "${FLUTTER_GENERATED_PATHS[@]}"
}
trap cleanup EXIT
export PYTHONPYCACHEPREFIX="$PYCACHE"

python3 "$ROOT/scripts/secret_scan.py" --root "$ROOT"
python3 "$ROOT/scripts/check_repo.py" --root "$ROOT"
python3 -m unittest discover -s "$ROOT/tests" -p 'test_*.py' -v

rsync -a --delete --exclude='.git/' --exclude='.pio/' "$ROOT/firmware/" "$PIO_WORK_DIR/"
pio run -d "$PIO_WORK_DIR" -e esp32dev -e esp32dev-experimental-compile -e esp32dev-actuators-only-compile
(
  cd "$FLUTTER_DIR"
  flutter pub get --enforce-lockfile
  dart format --output=none --set-exit-if-changed lib test
  flutter test
  flutter analyze
  flutter build apk --debug
)

cleanup
trap - EXIT
python3 "$ROOT/scripts/secret_scan.py" --root "$ROOT"
python3 "$ROOT/scripts/check_repo.py" --root "$ROOT"
echo 'Verification: PASS (static and build evidence only; no hardware/network claim)'
