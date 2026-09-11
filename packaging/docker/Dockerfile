# Dedicated server only (bitfighterd). No client, no SDL/OpenAL/PNG.
# Builder image is overridable so an older tag (bitfighter-022) can use
# ubuntu:22.04 without a second Dockerfile.
ARG BUILDER_IMAGE=ubuntu:24.04

FROM ${BUILDER_IMAGE} AS builder
ARG BUILDER_IMAGE
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      build-essential \
      cmake \
      ca-certificates \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

# Out-of-tree build: repo `build/` holds Android project files.
# No SDL → COMPILE_CLIENT=NO. In-tree LuaJIT/tomcrypt/sqlite.
RUN cmake -S . -B /tmp/bf-build \
      -DCMAKE_BUILD_TYPE=Release \
      -DLINUX_DATA_DIR=/app \
      -DMASTER_MINIMAL=ON \
      -DDISCORD=OFF \
      -DUSE_LEGACY_GL=ON \
 && cmake --build /tmp/bf-build --target bitfighterd tnlping -j"$(nproc)" \
 && test -x /src/exe/bitfighterd \
 && test -x /src/exe/tnlping \
 && ldd /src/exe/bitfighterd \
 && ldd /src/exe/tnlping

FROM ${BUILDER_IMAGE} AS runtime
ARG BUILDER_IMAGE
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      libstdc++6 \
      ca-certificates \
 && rm -rf /var/lib/apt/lists/* \
 && mkdir -p /app/bitfighter /data \
 && if getent passwd ubuntu >/dev/null; then \
      usermod -l bitfighter -d /data -s /usr/sbin/nologin ubuntu; \
      groupmod -n bitfighter ubuntu; \
    else \
      useradd --system --uid 1000 --home /data --shell /usr/sbin/nologin bitfighter; \
    fi

COPY --from=builder /src/exe/bitfighterd /app/bitfighterd
COPY --from=builder /src/exe/tnlping /app/tnlping
COPY --from=builder /src/resource/levels /app/bitfighter/levels
COPY --from=builder /src/resource/robots /app/bitfighter/robots
COPY --from=builder /src/resource/scripts /app/bitfighter/scripts
COPY --from=builder /src/resource/gamecontrollerdb.txt /app/bitfighter/gamecontrollerdb.txt
COPY --from=builder /src/resource/usergamecontrollerdb.txt /app/bitfighter/usergamecontrollerdb.txt
COPY packaging/docker/entrypoint.sh /app/entrypoint.sh

RUN chmod 755 /app/bitfighterd /app/tnlping /app/entrypoint.sh \
 && chown -R bitfighter:bitfighter /data

ENV HOME=/data \
    PATH="/app:${PATH}" \
    BITFIGHTER_PORT=28000

USER bitfighter
WORKDIR /app
VOLUME ["/data"]
EXPOSE 28000/udp

ENTRYPOINT ["/app/entrypoint.sh"]

HEALTHCHECK --interval=30s --timeout=10s --start-period=20s --retries=3 \
  CMD sh -c 'tnlping "127.0.0.1:${BITFIGHTER_PORT:-28000}"'
