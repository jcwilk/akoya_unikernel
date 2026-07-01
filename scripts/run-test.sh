#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "make test: idle-at-prompt gate on main kernel"
bash "${ROOT_DIR}/scripts/run-idle-at-prompt-test.sh"

echo "make test: transport-test image"
bash "${ROOT_DIR}/scripts/run-transport-test.sh"

echo "make test: all gates passed"
