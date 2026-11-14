# Dedicated server image

`bitfighterd` from this tree (protocol 42, release 023). A 022 client will not see or join it. Build the image yourself; nothing here is pushed to a registry.

```sh
docker build -f packaging/docker/Dockerfile -t bitfighterd:local .
```

One server on UDP 28000, listed on the stock master `107.172.71.173:25955`:

```sh
docker run -d --name bf1 --restart unless-stopped \
  -p 28000:28000/udp \
  -e BITFIGHTER_HOSTNAME='VPS one' \
  -e BITFIGHTER_HOSTDESCR='Evening game' \
  -e BITFIGHTER_SERVER_PASSWORD=play \
  -e BITFIGHTER_ADMIN_PASSWORD=admin \
  -e BITFIGHTER_LEVEL_CHANGE_PASSWORD=levels \
  -e BITFIGHTER_MAX_PLAYERS=16 \
  -v bf1:/data \
  bitfighterd:local
```

Several on one host. The container port and the published port are the same, because that is the port the process reports:

```sh
docker run -d --name bf2 -p 28001:28001/udp \
  -e BITFIGHTER_PORT=28001 \
  -e BITFIGHTER_HOSTNAME='VPS two' \
  -e BITFIGHTER_SERVER_PASSWORD=play \
  -v bf2:/data \
  bitfighterd:local
```

Or three at once:

```sh
cd packaging/docker
cp .env.example .env
docker compose up -d --build
```

A private server that does not register. `-master` is pointed at `127.0.0.1:9` inside the container:

```sh
docker run --rm -p 127.0.0.1:28000:28000/udp \
  -e BITFIGHTER_REGISTER_MASTER=0 \
  -e BITFIGHTER_SERVER_PASSWORD=lan-secret \
  -e BITFIGHTER_HOSTNAME=LocalTest \
  bitfighterd:local
```

`tnlping 127.0.0.1:28000` from inside the container is the healthcheck. It only checks that the UDP socket answers.

## Data

`/data` is `-rootdatadir` and the volume to keep.

| Path | What |
| --- | --- |
| `/data/bitfighter.ini` | Settings written by the server |
| `/data/levels/` | Level files. Stock levels are copied here on first start if the directory is empty |
| `/data/robots/`, `/data/scripts/` | Same for bots and Lua scripts |
| `/app/bitfighter/levels/` | Read-only stock levels shipped in the image |

Mount a directory of your own levels over `/data/levels`, or set `BITFIGHTER_LEVELDIR` to another folder. `BITFIGHTER_LEVELS` is a space-separated list of level filenames passed as `-levels` and must be the last arguments, so do not also pass `-levels` yourself.

The process runs as uid 1000. Bind mounts need to be writable by that user.

## Env

| Env | Flag |
| --- | --- |
| `BITFIGHTER_PORT` (default 28000) | `-hostaddr IP:Any:$PORT` |
| `BITFIGHTER_HOSTADDR` | replaces that `-hostaddr` |
| `BITFIGHTER_HOSTNAME` | `-hostname` |
| `BITFIGHTER_HOSTDESCR` | `-hostdescr` |
| `BITFIGHTER_SERVER_PASSWORD` | `-serverpassword` |
| `BITFIGHTER_ADMIN_PASSWORD` | `-adminpassword` |
| `BITFIGHTER_OWNER_PASSWORD` | `-ownerpassword` |
| `BITFIGHTER_LEVEL_CHANGE_PASSWORD` | `-levelchangepassword` |
| `BITFIGHTER_MAX_PLAYERS` | `-maxplayers` |
| `BITFIGHTER_LEVELDIR` | `-leveldir` |
| `BITFIGHTER_LEVELS` | `-levels ...` (last) |
| `BITFIGHTER_REGISTER_MASTER` (default 1) | `0` sends `-master IP:127.0.0.1:9` |
| `BITFIGHTER_MASTER` (default `IP:107.172.71.173:25955`) | `-master` when registration is on |

Extra arguments are appended before `-levels`. `-dedicated` and `-rootdatadir /data` are always set.

## Relay

The browser WebSocket-to-UDP relay in `tools/udp-ws-relay` can run on this same VPS later. It is not part of this image. `packaging/docker/compose.yaml` has it commented out; it wants host networking so reply packets come back to the sockets it binds.
