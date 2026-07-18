FROM ubuntu:24.04

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        ca-certificates \
        sudo \
        curl \
        git \
        make \
        cmake \
        pkg-config \
        gdb \
        lldb \
        python3 \
        python3-pip \
        pipx \
    && rm -rf /var/lib/apt/lists/*

RUN echo "ubuntu ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/ubuntu \
    && chmod 0440 /etc/sudoers.d/ubuntu

COPY .devcontainer/base/scripts/info.sh /usr/local/lib/devcontainer/info.sh

COPY .devcontainer/base/scripts/welcome.sh /usr/local/bin/welcome.sh

RUN chmod +x /usr/local/bin/welcome.sh \
    && echo '/usr/local/bin/welcome.sh' >> /etc/bash.bashrc

USER ubuntu
WORKDIR /home/ubuntu

ENV PATH="/home/ubuntu/.local/bin:${PATH}"

RUN pipx install conan \
    && conan profile detect --force

RUN touch ~/.sudo_as_admin_successful
