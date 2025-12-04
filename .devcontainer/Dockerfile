FROM espressif/idf:v5.2

ARG DEBIAN_FRONTEND=nointeractive
ARG CONTAINER_USER=develop
ARG USER_UID=1000
ARG USER_GID=$USER_UID


# Install additional tools
RUN apt-get update && apt-get install -y -q \
    git \
    wget \
    flex \
    bison \
    gperf \
    python3 \
    python3-pip \
    python3-venv \
    cmake \
    ninja-build \
    ccache \
    libffi-dev \
    libssl-dev \
    dfu-util \
    libusb-1.0-0 \
    sudo \
    build-essential cmake ninja-build python3 python3-virtualenv git \
    wget flex bison gperf python3 python3-pip python3-setuptools libffi-dev libssl-dev libncurses-dev sudo 
    && rm -rf /var/lib/apt/lists/*

RUN if ! getent group $USER_GID; then groupadd --gid $USER_GID $CONTAINER_USER; fi \
    && if ! id -u $USER_UID > /dev/null 2>&1; then adduser --uid $USER_UID --gid $USER_GID --disabled-password --gecos "" ${CONTAINER_USER}; fi \
    && usermod -aG sudo $CONTAINER_USER \
    && echo "$CONTAINER_USER:$CONTAINER_USER" | chpasswd \
    && echo $CONTAINER_USER ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$CONTAINER_USER \
    && chmod 0440 /etc/sudoers.d/$CONTAINER_USER
  
  USER ${CONTAINER_USER}
  ENV USER=${CONTAINER_USER}

# Set up working directory
WORKDIR /workspace

# Keep container running
CMD ["/bin/bash"]
