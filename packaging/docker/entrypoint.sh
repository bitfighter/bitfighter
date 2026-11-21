#!/bin/bash
# Map env vars onto bitfighterd's existing CLI flags. Always dedicated.
# Data (INI, logs, levels) lives in /data, which is the volume to persist.
set -euo pipefail

PORT="${BITFIGHTER_PORT:-28000}"
DATA="${BITFIGHTER_DATA:-/data}"
INSTALL="${BITFIGHTER_INSTALL:-/app/bitfighter}"

mkdir -p "$DATA"
# Skip the engine's first-launch copy into $HOME/.bitfighter. We seed $DATA.
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

# Default on: announce to the stock master. Set BITFIGHTER_REGISTER_MASTER=0
# to aim -master at an unroutable address so the server stays private.
register="${BITFIGHTER_REGISTER_MASTER:-1}"
case "$register" in
  0|no|false|off|NO|FALSE|No)
    args+=(-master "IP:127.0.0.1:9")
    ;;
  *)
    args+=(-master "${BITFIGHTER_MASTER:-IP:107.172.71.173:25955}")
    ;;
esac

[[ -n "${BITFIGHTER_HOSTNAME:-}" ]] && args+=(-hostname "$BITFIGHTER_HOSTNAME")
[[ -n "${BITFIGHTER_HOSTDESCR:-}" ]] && args+=(-hostdescr "$BITFIGHTER_HOSTDESCR")
[[ -n "${BITFIGHTER_SERVER_PASSWORD:-}" ]] && args+=(-serverpassword "$BITFIGHTER_SERVER_PASSWORD")
[[ -n "${BITFIGHTER_ADMIN_PASSWORD:-}" ]] && args+=(-adminpassword "$BITFIGHTER_ADMIN_PASSWORD")
[[ -n "${BITFIGHTER_OWNER_PASSWORD:-}" ]] && args+=(-ownerpassword "$BITFIGHTER_OWNER_PASSWORD")
[[ -n "${BITFIGHTER_LEVEL_CHANGE_PASSWORD:-}" ]] && args+=(-levelchangepassword "$BITFIGHTER_LEVEL_CHANGE_PASSWORD")
[[ -n "${BITFIGHTER_MAX_PLAYERS:-}" ]] && args+=(-maxplayers "$BITFIGHTER_MAX_PLAYERS")
[[ -n "${BITFIGHTER_LEVELDIR:-}" ]] && args+=(-leveldir "$BITFIGHTER_LEVELDIR")

# Caller args, then -levels last. -levels consumes every remaining argument.
args+=("$@")
if [[ -n "${BITFIGHTER_LEVELS:-}" ]]; then
  # shellcheck disable=SC2206
  level_args=(${BITFIGHTER_LEVELS})
  args+=(-levels "${level_args[@]}")
fi

exec "${args[@]}"
