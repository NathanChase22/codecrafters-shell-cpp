export VCPKG_FORCE_SYSTEM_BINARIES=0

#!/bin/bash
apt-get update
apt-get install \
    cmake \
    curl \
    zip \
    unzip \
    tar \
    ninja-build \
    build-essential \
    pkg-config \
    libreadline-dev \

cd /
git clone https://github.com/microsoft/vcpkg.git
touch .shell_history