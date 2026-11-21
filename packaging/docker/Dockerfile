# Dedicated server only (bitfighterd). No client, no SDL, no OpenAL.
# Builder and runtime stay on the same Debian release so libstdc++ matches.
# Override both together if an older tag needs a different base.
ARG BUILDER_IMAGE=debian:bookworm
ARG RUNTIME_IMAGE=debian:bookworm-slim

FROM ${BUILDER_IMAGE} AS builder
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      build-essential \
      cmake \
      ca-certificates \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

# Out-of-tree build: the repo's build/ directory is unrelated.
# No OpenAL in this image, so the dedicated server compiles with BF_NO_AUDIO.
RUN cmake -S . -B /tmp/bf-build \
      -DCMAKE_BUILD_TYPE=Release \
      -DLINUX_DATA_DIR=/app \
      -DDISCORD=OFF \
 && cmake --build /tmp/bf-build --target bitfighterd tnlping -j"$(nproc)" \
 && test -x /src/exe/bitfighterd \
 && test -x /src/exe/tnlping \
 && ldd /src/exe/bitfighterd \
 && ldd /src/exe/tnlping

FROM ${RUNTIME_IMAGE} AS runtime
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      libstdc++6 \
      bash \
 && rm -rf /var/lib/apt/lists/* \
 && mkdir -p /app/bitfighter /data \
 && useradd --uid 1000 --no-create-home --home-dir /data \
      --shell /usr/sbin/nologin bitfighter

COPY --from=builder /src/exe/bitfighterd /app/bitfighterd
COPY --from=builder /src/exe/tnlping /app/tnlping
COPY --from=builder /src/resource/levels /app/bitfighter/levels
COPY --from=builder /src/resource/robots /app/bitfighter/robots
COPY --from=builder /src/resource/scripts /app/bitfighter/scripts
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
  CMD bash -c 'tnlping "127.0.0.1:${BITFIGHTER_PORT:-28000}"'
