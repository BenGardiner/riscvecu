FROM renode:1.16

USER root

RUN apt-get update && apt-get install -y \
    build-essential \
    curl

ARG XPACK_VERSION="15.2.0-1"
# Download and extract the Linux xPack tarball to /opt/xpack/riscv
# See releases: https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases
RUN mkdir -p /opt/xpack/riscv && \
    curl -L "https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases/download/v${XPACK_VERSION}/xpack-riscv-none-elf-gcc-${XPACK_VERSION}-linux-x64.tar.gz" \
      | tar -xz -C /opt/xpack/riscv
ENV PATH="/opt/xpack/riscv/xpack-riscv-none-elf-gcc-${XPACK_VERSION}/bin:${PATH}"

# Quick smoke test
RUN riscv-none-elf-gcc --version

# Dependencies for runtime of renode with firmware
RUN apt-get update && apt-get install -y \
    iproute2 \
    can-utils \
    kmod \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
