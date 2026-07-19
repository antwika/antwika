ARG BASE_VERSION=latest
FROM ghcr.io/antwika/antwika-dev-base:${BASE_VERSION}

USER root

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        gcc \
        g++ \
    && rm -rf /var/lib/apt/lists/*

ENV CPP_TOOLCHAIN=gnu

COPY .devcontainer/gnu/scripts/toolchain-info.sh /usr/local/bin/toolchain-info.sh

RUN chmod +x /usr/local/bin/toolchain-info.sh \
    && echo '/usr/local/bin/toolchain-info.sh' >> /etc/bash.bashrc

USER ubuntu

RUN conan profile detect --force
