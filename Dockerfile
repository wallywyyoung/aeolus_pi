FROM debian:trixie-slim AS base

ENV DEBIAN_FRONTEND=noninteractive

WORKDIR /workspace

COPY . .

RUN chmod +x ./init.sh && ./init.sh

ENTRYPOINT ["make", "-C", "buildroot"]
