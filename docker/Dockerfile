FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    git cmake ninja-build gperf \
    ccache dfu-util device-tree-compiler wget \
    python3 python3-pip python3-venv python3-dev \
    xz-utils file make gcc g++ \
    qemu-system-arm \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install --no-cache-dir west pyelftools pyyaml

ENV ZEPHYR_SDK_VERSION=0.16.8
ENV ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk

RUN wget -q https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${ZEPHYR_SDK_VERSION}/zephyr-sdk-${ZEPHYR_SDK_VERSION}_linux-aarch64.tar.xz -O /tmp/zephyr-sdk.tar.xz && \
    tar -xf /tmp/zephyr-sdk.tar.xz -C /opt && \
    rm -rf ${ZEPHYR_SDK_INSTALL_DIR} && \
    mv /opt/zephyr-sdk-${ZEPHYR_SDK_VERSION} ${ZEPHYR_SDK_INSTALL_DIR} && \
    ${ZEPHYR_SDK_INSTALL_DIR}/setup.sh -t all -h -c && \
    rm -f /tmp/zephyr-sdk.tar.xz

WORKDIR /work
