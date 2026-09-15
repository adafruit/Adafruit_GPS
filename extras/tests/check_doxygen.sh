#!/usr/bin/env bash
# Match ci-arduino's documentation check without cloning or publishing gh-pages.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
build="$(mktemp -d "${TMPDIR:-/tmp}/gps-doxygen.XXXXXX")"
echo "Doxygen check output: $build"

# ci/doxy_gen_and_deploy.sh uses this binary, not the system Doxygen version.
curl --fail --location --silent --show-error --connect-timeout 15 --max-time 120 \
  https://cdn-learn.adafruit.com/assets/assets/000/067/405/original/doxygen-1.8.13.linux.bin.tar.gz \
  --output "$build/doxygen.tar.gz"
tar -xzf "$build/doxygen.tar.gz" -C "$build" doxygen-1.8.13/bin/doxygen

if [[ -f "$root/Doxyfile" ]]; then
  cp "$root/Doxyfile" "$build/Doxyfile"
else
  curl --fail --location --silent --show-error --connect-timeout 15 --max-time 120 \
    https://raw.githubusercontent.com/adafruit/travis-ci-arduino/master/Doxyfile.default \
    --output "$build/Doxyfile"
fi
cat >> "$build/Doxyfile" <<EOF

PROJECT_NAME = "Adafruit GPS Library"
HTML_OUTPUT = "$build/html"
INPUT = "$root/src"
EOF

cd "$root"
if ! "$build/doxygen-1.8.13/bin/doxygen" "$build/Doxyfile" > "$build/doxygen.log" 2>&1; then
  cat "$build/doxygen.log" >&2
  exit 1
fi
# The deployment job rejects any output, even when Doxygen exits successfully.
if [[ -s "$build/doxygen.log" ]]; then
  cat "$build/doxygen.log" >&2
  exit 1
fi
if [[ ! -s "$build/html/index.html" ]]; then
  echo "Doxygen did not generate html/index.html" >&2
  exit 1
fi
echo "Doxygen 1.8.13 passed with no warnings."
