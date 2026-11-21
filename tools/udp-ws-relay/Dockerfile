# Private WebSocket-to-UDP relay. Do not publish this without a port allowlist.
# Host networking lets return packets reach the UDP sockets the relay binds.
#
#   docker build -t bitfighter-udp-ws-relay tools/udp-ws-relay
#   docker run --rm --network host -e RELAY_ALLOW_PORTS=25955,28000 bitfighter-udp-ws-relay
FROM python:3.12-alpine
COPY relay.py /relay.py
EXPOSE 28080/tcp
ENTRYPOINT ["python3", "/relay.py"]
CMD ["28080"]
