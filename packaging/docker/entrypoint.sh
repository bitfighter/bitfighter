#!/bin/bash
# Map env vars onto bitfighterd CLI flags. Always dedicated; data lives in /data.
# -version/-help still go through this path: the engine starts Lua before
# printing those directives, so scripts must be seeded first.
set -euo pipefail

PORT="${BITFIGHTER_PORT:-28000}"
DATA="${BITFIGHTER_DATA:-/data}"
INSTALL="${BITFIGHTER_INSTALL:-/app/bitfighter}"

mkdir -p "$DATA"
# Skip C++ first-launch copy into $HOME/.bitfighter; seed $DATA ourselves.
mkdir -p "${HOME:-/data}/.bitfighter"

for d in levels robots scripts; do
  if [[ ! -d "$DATA/$d" ]] || [[ -z "$(ls -A "$DATA/$d" 2>/dev/null || true)" ]]; then
    mkdir -p "$DATA/$d"
    if [[ -d "$INSTALL/$d" ]]; then
      cp -a "$INSTALL/$d/." "$DATA/$d/"
    fi
  fi
done

args=(
  /app/bitfighterd
  -dedicated
  -rootdatadir "$DATA"
  -hostaddr "${BITFIGHTER_HOSTADDR:-IP:Any:${PORT}}"
)

[[ -n "${BITFIGHTER_HOSTNAME:-}" ]] && args+=(-hostname "$BITFIGHTER_HOSTNAME")
[[ -n "${BITFIGHTER_HOSTDESCR:-}" ]] && args+=(-hostdescr "$BITFIGHTER_HOSTDESCR")
[[ -n "${BITFIGHTER_ADMIN_PASSWORD:-}" ]] && args+=(-adminpassword "$BITFIGHTER_ADMIN_PASSWORD")
[[ -n "${BITFIGHTER_OWNER_PASSWORD:-}" ]] && args+=(-ownerpassword "$BITFIGHTER_OWNER_PASSWORD")
[[ -n "${BITFIGHTER_MAX_PLAYERS:-}" ]] && args+=(-maxplayers "$BITFIGHTER_MAX_PLAYERS")

args+=("$@")
exec "${args[@]}"
