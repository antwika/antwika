FROM base:latest

USER root

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        clang \
    && rm -rf /var/lib/apt/lists/*

ENV CPP_TOOLCHAIN=clang

COPY .devcontainer/clang/scripts/toolchain-info.sh /usr/local/bin/toolchain-info.sh

RUN chmod +x /usr/local/bin/toolchain-info.sh \
    && echo '/usr/local/bin/toolchain-info.sh' >> /etc/bash.bashrc

USER ubuntu

RUN conan profile detect --force
