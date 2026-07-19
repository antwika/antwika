ARG BASE_VERSION=latest
FROM ghcr.io/antwika/antwika-dev-base:${BASE_VERSION}

USER root

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
       mingw-w64 \
    && rm -rf /var/lib/apt/lists/*

ENV CPP_TOOLCHAIN=mingw

COPY .devcontainer/mingw/scripts/toolchain-info.sh /usr/local/bin/toolchain-info.sh

RUN chmod +x /usr/local/bin/toolchain-info.sh \
    && echo '/usr/local/bin/toolchain-info.sh' >> /etc/bash.bashrc

USER ubuntu

RUN conan profile detect --force
