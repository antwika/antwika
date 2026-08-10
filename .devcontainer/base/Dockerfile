FROM ubuntu:26.04

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
        gcovr \
        ccache \
    && rm -rf /var/lib/apt/lists/*

ENV CCACHE_SLOPPINESS=pch_defines,time_macros

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        libasound2-dev \
        libegl-dev \
        libfontenc-dev \
        libgl-dev \
        libice-dev \
        libsm-dev \
        libudev-dev \
        libx11-dev \
        libx11-xcb-dev \
        libxau-dev \
        libxaw7-dev \
        libxcb-composite0-dev \
        libxcb-cursor-dev \
        libxcb-dri2-0-dev \
        libxcb-dri3-dev \
        libxcb-ewmh-dev \
        libxcb-glx0-dev \
        libxcb-icccm4-dev \
        libxcb-image0-dev \
        libxcb-keysyms1-dev \
        libxcb-present-dev \
        libxcb-randr0-dev \
        libxcb-render-util0-dev \
        libxcb-render0-dev \
        libxcb-res0-dev \
        libxcb-shape0-dev \
        libxcb-sync-dev \
        libxcb-util-dev \
        libxcb-xfixes0-dev \
        libxcb-xinerama0-dev \
        libxcb-xkb-dev \
        libxcomposite-dev \
        libxcursor-dev \
        libxdamage-dev \
        libxdmcp-dev \
        libxext-dev \
        libxfixes-dev \
        libxi-dev \
        libxinerama-dev \
        libxkbfile-dev \
        libxmu-dev \
        libxmuu-dev \
        libxpm-dev \
        libxrandr-dev \
        libxrender-dev \
        libxres-dev \
        libxss-dev \
        libxt-dev \
        libxtst-dev \
        libxv-dev \
        libxxf86vm-dev \
        uuid-dev \
        xvfb \
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
