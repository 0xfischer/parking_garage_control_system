FROM espressif/idf:release-v5.2

ARG DEBIAN_FRONTEND=noninteractive
ARG CONTAINER_USER=develop
ARG USER_UID=1000
ARG USER_GID=$USER_UID

# Tools
RUN apt-get update && apt-get install -y -q --no-install-recommends \
    git wget curl flex bison gperf sudo ca-certificates \
    python3 python3-pip python3-venv python3-setuptools python3-virtualenv \
    dfu-util libusb-1.0-0 \
    build-essential cmake ninja-build ccache \
    libffi-dev libssl-dev libncurses-dev \
    clang-format clang-tidy gcovr lcov \
    && rm -rf /var/lib/apt/lists/*

# User anlegen
RUN if ! getent group $USER_GID >/dev/null; then groupadd --gid $USER_GID $CONTAINER_USER; fi \
    && if ! id -u $USER_UID >/dev/null 2>&1; then adduser --uid $USER_UID --gid $USER_GID --disabled-password --gecos "" ${CONTAINER_USER}; fi \
    && usermod -aG sudo,dialout $CONTAINER_USER \
    && echo "$CONTAINER_USER:$CONTAINER_USER" | chpasswd \
    && echo "$CONTAINER_USER ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/$CONTAINER_USER \
    && chmod 0440 /etc/sudoers.d/$CONTAINER_USER

# Zu User wechseln für Tool-Installationen
USER ${CONTAINER_USER}
ENV USER=${CONTAINER_USER}
ENV HOME=/home/${CONTAINER_USER}
ENV PATH="$HOME/.local/bin:$PATH"

WORKDIR /workspace

# Wokwi CLI (als User installieren)
RUN wget -qO- https://wokwi.com/ci/install.sh | sh

# act (install to /usr/local/bin so it's in PATH)
RUN curl -s https://raw.githubusercontent.com/nektos/act/master/install.sh | sudo bash -s -- -b /usr/local/bin

# pre-commit (als User installieren)
RUN pip3 install --no-cache-dir --user pre-commit

# Git safe.directory and editor configuration
RUN git config --global --add safe.directory /opt/esp/idf \
    && git config --global --add safe.directory /workspace \
    && git config --global core.editor "vim"

# Ensure clean bashrc without old ESP-IDF paths
RUN echo "" > ~/.bashrc \
    && echo "# ESP-IDF Environment" >> ~/.bashrc \
    && echo "export IDF_PATH=/opt/esp/idf" >> ~/.bashrc \
    && echo "export IDF_TOOLS_PATH=/opt/esp" >> ~/.bashrc \
    && echo "source \$IDF_PATH/export.sh > /dev/null 2>&1" >> ~/.bashrc

ENTRYPOINT [ "/opt/esp/entrypoint.sh" ]
CMD ["/bin/bash"]