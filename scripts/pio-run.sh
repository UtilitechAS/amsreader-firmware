#!/usr/bin/env bash
set -euo pipefail

ARGS=("$@")
if [[ ${#ARGS[@]} -eq 0 ]]; then
  ARGS=("run")
fi

if [[ "$(uname -s)" == "Darwin" && "$(uname -m)" == "arm64" ]]; then
  if ! /usr/bin/arch -x86_64 /usr/bin/true >/dev/null 2>&1; then
    cat <<'EOF'
Rosetta 2 does not appear to be installed or is disabled. Install it with:
  softwareupdate --install-rosetta --agree-to-license
EOF
    exit 1
  fi

  PIO_X86_DIR="${HOME}/.platformio/penv-x86"
  PIO_BIN="${PIO_X86_DIR}/bin/platformio"

  if [[ ! -x "${PIO_BIN}" ]]; then
    echo "Setting up x86 PlatformIO environment in ${PIO_X86_DIR}..."
    /usr/bin/arch -x86_64 /usr/bin/python3 -m venv "${PIO_X86_DIR}"
    /usr/bin/arch -x86_64 "${PIO_X86_DIR}/bin/pip" install --upgrade pip wheel platformio
  fi

  exec /usr/bin/arch -x86_64 "${PIO_BIN}" "${ARGS[@]}"
else
  exec pio "${ARGS[@]}"
fi
