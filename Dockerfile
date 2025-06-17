FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    wget \
    libgl1-mesa-dev \
    libxkbcommon-x11-0 \
    libxcb-cursor-dev \
    clang-tidy \
    fuse \
    libfuse2 \
    imagemagick \
    patchelf \
    python3 \
    python3-pip \
    python3-venv \
    qt6-base-dev \
    qt6-tools-dev \
    qt6-tools-dev-tools \
    qt6-l10n-tools \
    make \
    libssl-dev \
    zlib1g-dev \
    libbz2-dev \
    libreadline-dev \
    libsqlite3-dev \
    llvm \
    libncursesw5-dev \
    xz-utils \
    tk-dev \
    libxml2-dev \
    libxmlsec1-dev \
    libffi-dev \
    liblzma-dev \
    sudo \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

RUN pip3 install --no-cache-dir pipenv
    
RUN curl https://pyenv.run | bash

RUN echo 'if [ ! -d /workspace/.pyenv ]; then' >> ~/.bashrc && \
    echo '  mv $HOME/.pyenv /workspace/.pyenv' >> ~/.bashrc && \
    echo 'fi' >> ~/.bashrc && \
    echo 'export PYENV_ROOT="/workspace/.pyenv"' >> ~/.bashrc && \
    echo 'export PATH="$PYENV_ROOT/bin:$PATH"' >> ~/.bashrc && \
    echo 'eval "$(pyenv init - bash)"' >> ~/.bashrc && \
    echo 'eval "$(pyenv virtualenv-init -)"' >> ~/.bashrc && \
    echo 'if [ -f "/workspace/Pipfile" ]; then' >> ~/.bashrc && \
    echo '  PY_VER=$(awk -F\" '"'"'/python_version/ { print $2 }'"'"' "/workspace/Pipfile")' >> ~/.bashrc && \
    echo '  if [ -n "$PY_VER" ]; then' >> ~/.bashrc && \
    echo '    if ! pyenv versions --bare | grep -qx "$PY_VER"; then' >> ~/.bashrc && \
    echo '      echo "[pyenv] Checking for Python $PY_VER..."' >> ~/.bashrc && \
    echo '      pyenv install --skip-existing "$PY_VER"' >> ~/.bashrc && \
    echo '    fi' >> ~/.bashrc && \
    echo '    if [ "$(pyenv version-name)" != "$PY_VER" ]; then' >> ~/.bashrc && \
    echo '      pyenv shell "$PY_VER"' >> ~/.bashrc && \
    echo '    fi' >> ~/.bashrc && \
    echo '  fi' >> ~/.bashrc && \
    echo 'fi' >> ~/.bashrc

WORKDIR /workspace
