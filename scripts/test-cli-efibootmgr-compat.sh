#!/bin/bash
# Compare qefibootmgr CLI behavior with RedHat's efibootmgr.
# Intended to run against a dummy-backend build:
#   cmake -DAPP_DATA_DUMMY_BACKEND=ON -DBUILD_CLI_UTILITY=ON ...
#   cmake --build . --target qefibootmgr
#   scripts/test-cli-efibootmgr-compat.sh /path/to/qefibootmgr

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
QEFIBOOTMGR="${1:-}"

if [ -z "${QEFIBOOTMGR}" ]; then
    echo "Usage: $0 /path/to/qefibootmgr" >&2
    exit 2
fi

if [ ! -x "${QEFIBOOTMGR}" ]; then
    echo "ERROR: qefibootmgr not found or not executable: ${QEFIBOOTMGR}" >&2
    exit 1
fi

PYTHON=""
for candidate in python3 python; do
    if command -v "${candidate}" >/dev/null 2>&1; then
        PYTHON="${candidate}"
        break
    fi
done

if [ -z "${PYTHON}" ]; then
    echo "ERROR: python3 is required to run CLI compatibility tests" >&2
    exit 1
fi

exec "${PYTHON}" "${REPO_ROOT}/tests/cli/test_qefibootmgr_compat.py" \
    --qefibootmgr "${QEFIBOOTMGR}"
