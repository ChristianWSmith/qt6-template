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
    && apt-get clean && rm -rf /var/lib/apt/lists/*

RUN pip3 install --no-cache-dir pipenv

RUN apt-get update && apt-get install -y \
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
    && apt-get clean && rm -rf /var/lib/apt/lists/*
    
RUN curl https://pyenv.run | bash && \
    echo 'export PYENV_ROOT="$HOME/.pyenv"' >> ~/.bashrc && \
    echo 'export PATH="$PYENV_ROOT/bin:$PATH"' >> ~/.bashrc && \
    echo 'eval "$(pyenv init --path)"' >> ~/.bashrc && \
    echo 'eval "$(pyenv virtualenv-init -)"' >> ~/.bashrc && \
    echo '' >> ~/.bashrc && \
    echo '# Auto-switch Python version based on Pipfile' >> ~/.bashrc && \
    echo 'if [ -f "/workspace/Pipfile" ]; then' >> ~/.bashrc && \
    echo '  PY_VER=$(grep -Po "(?<=python_version\\s*=\\s*\")[^\"]+" "/workspace/Pipfile")' >> ~/.bashrc && \
    echo '  if [ -n "$PY_VER" ]; then' >> ~/.bashrc && \
    echo '    if ! pyenv versions --bare | grep -qx "$PY_VER"; then' >> ~/.bashrc && \
    echo '      echo "[pyenv] Installing Python $PY_VER..."' >> ~/.bashrc && \
    echo '      pyenv install "$PY_VER"' >> ~/.bashrc && \
    echo '    fi' >> ~/.bashrc && \
    echo '    pyenv shell "$PY_VER"' >> ~/.bashrc && \
    echo '  fi' >> ~/.bashrc && \
    echo 'fi' >> ~/.bashrc

RUN apt-get update && \
    apt-get install -y sudo && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
