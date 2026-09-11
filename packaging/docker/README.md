# Dedicated server image

`bitfighterd` as a container, published to `ghcr.io/bitfighter/bitfighterd`.

## Which tag

| Tag | Protocol | Who can join |
|---|---|---|
| `:022` | 41 | Current public clients. **Pin this.** |
| `:023` | 42 | 023 clients only. 022 players will not see the server. |
| `:master` | whatever `master` is | Dev snapshots. Not for public games. |

There is no `:latest`. Mixing 022 and 023 on the same port does not work.

Images are **linux/amd64** (CI). Compose pins that platform so an ARM host still runs the published image.

`:022` is built from the `bitfighter-022` tag with this Dockerfile (022 has no Docker files of its own). First publish is a manual `workflow_dispatch` after the image is known to compile.

## Run

```sh
cd packaging/docker
cp .env.example .env
# set BITFIGHTER_OWNER_PASSWORD and BITFIGHTER_ADMIN_PASSWORD
docker compose up -d
```

That pulls `:022` and publishes **UDP 28000**. The server lists itself on the official master automatically.

Optional 023 on host UDP **28001**:

```sh
docker compose --profile 023 up -d
```

Build locally instead of pulling:

```sh
docker compose build
docker compose up -d
```

Or without compose:

```sh
docker build -f packaging/docker/Dockerfile -t bitfighterd:local .
docker run --rm bitfighterd:local -version   # prints "Bitfighter 023" on current master
docker run --rm -p 28000:28000/udp \
  -e BITFIGHTER_HOSTNAME=smoke \
  -e BITFIGHTER_OWNER_PASSWORD=changeme \
  bitfighterd:local
```

## Network

Bitfighter is TNL over **UDP**, IPv4. Caddy/HTTP is not involved.

- Open **28000/udp** (and **28001/udp** if running 023).
- Docker publishes that port through docker-proxy; also allow it on the host firewall (e.g. UFW).

## Data and config

`/data` is the writable root (`-rootdatadir /data`): `bitfighter.ini`, logs, `levels/`, `robots/`, `scripts/`. Stock files are copied in on first start if those dirs are empty.

`AllowDataConnections` defaults to **No**. Leave it off unless you need admin upload/download of levels and bots.

Named volumes pick up uid 1000 from the image. Bind-mounts should be owned by uid 1000.

## Env → flags

| env | flag |
|---|---|
| `BITFIGHTER_HOSTNAME` | `-hostname` |
| `BITFIGHTER_HOSTDESCR` | `-hostdescr` |
| `BITFIGHTER_PORT` (default 28000) | `-hostaddr IP:Any:$PORT` |
| `BITFIGHTER_ADMIN_PASSWORD` | `-adminpassword` |
| `BITFIGHTER_OWNER_PASSWORD` | `-ownerpassword` |
| `BITFIGHTER_MAX_PLAYERS` | `-maxplayers` |

Extra `docker run` args are appended as-is. `-dedicated` and `-rootdatadir /data` are always passed.

Healthcheck: `tnlping 127.0.0.1:$BITFIGHTER_PORT`.
