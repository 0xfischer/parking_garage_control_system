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

# Wokwi CLI
RUN wget -qO- https://wokwi.com/ci/install.sh | sh

# act (benötigt Docker-CLI + /var/run/docker.sock im Devcontainer)
RUN curl -s https://raw.githubusercontent.com/nektos/act/master/install.sh | bash

# pre-commit
RUN pip3 install --no-cache-dir pre-commit

USER ${CONTAINER_USER}
ENV USER=${CONTAINER_USER}
ENV HOME=/home/${CONTAINER_USER}
ENV PATH="$HOME/.local/bin:$PATH"

WORKDIR /workspace

# Git safe.directory und optionales IDF-Export in interaktiven Shells
RUN /usr/bin/git config --global --add safe.directory /opt/esp/idf
# Optional, EntryPoint setzt IDF-Umgebung ohnehin
# RUN echo "source /opt/esp/idf/export.sh > /dev/null 2>&1" >> ~/.bashrc

ENTRYPOINT [ "/opt/esp/entrypoint.sh" ]
CMD ["/bin/bash"]