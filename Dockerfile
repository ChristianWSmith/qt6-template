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
    locales \
    locales-all \
    inkscape \
    libfontenc-dev \
    libice-dev \
    libsm-dev \
    libx11-xcb-dev \
    libxaw7-dev \
    libxcb-composite0 \
    libxcb-composite0-dev \
    libxcb-dri2-0-dev \
    libxcb-dri3-dev \
    libxcb-ewmh-dev \
    libxcb-ewmh2 \
    libxcb-glx0-dev \
    libxcb-icccm4-dev \
    libxcb-keysyms1-dev \
    libxcb-present-dev \
    libxcb-randr0-dev \
    libxcb-res0 \
    libxcb-res0-dev \
    libxcb-shape0-dev \
    libxcb-sync-dev \
    libxcb-xfixes0-dev \
    libxcb-xinerama0 \
    libxcb-xinerama0-dev \
    libxcb-xkb-dev \
    libxcomposite-dev \
    libxcursor-dev \
    libxdamage-dev \
    libxfixes-dev \
    libxi-dev \
    libxinerama-dev \
    libxkbfile-dev \
    libxmu-dev \
    libxmu-headers \
    libxmuu-dev \
    libxpm-dev \
    libxrandr-dev \
    libxres-dev \
    libxres1 \
    libxt-dev \
    libxtst-dev \
    libxv-dev \
    libxxf86vm-dev \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

RUN pip3 install --no-cache-dir --upgrade pip pipenv

RUN groupadd -g 1000 devgroup && \
    useradd -m -u 1000 -g devgroup devuser && \
    echo 'devuser ALL=(ALL) NOPASSWD:ALL' > /etc/sudoers && \
    chmod 0440 /etc/sudoers

USER devuser

RUN curl https://pyenv.run | bash

RUN echo 'export PIPENV_VENV_IN_PROJECT=1' >> ~/.bashrc && \
    echo 'parse_git_branch() {' >> ~/.bashrc && \
    echo '  if git rev-parse --is-inside-work-tree &>/dev/null; then' >> ~/.bashrc && \
    echo '    branch=$(git symbolic-ref --short HEAD 2>/dev/null || git describe --tags --exact-match 2>/dev/null || echo "detached")' >> ~/.bashrc && \
    echo '    echo " ($branch)"' >> ~/.bashrc && \
    echo '  fi' >> ~/.bashrc && \
    echo '}' >> ~/.bashrc && \
    echo 'export PS1="\[\033[1;34m\]\$(pwd | sed '\''s|^${WORKSPACE%/}|[workspace]|'\'')\[\033[0m\]\[\033[1;32m\]\$(parse_git_branch)\[\033[0m\] \$ "' >> ~/.bashrc && \
    echo 'if [ ! -d "${WORKSPACE}/.pyenv" ]; then' >> ~/.bashrc && \
    echo '  mv "${HOME}/.pyenv" "${WORKSPACE}/.pyenv"' >> ~/.bashrc && \
    echo 'fi' >> ~/.bashrc && \
    echo 'export PYENV_ROOT="${WORKSPACE}/.pyenv"' >> ~/.bashrc && \
    echo 'export PATH="${PYENV_ROOT}/bin:${PATH}"' >> ~/.bashrc && \
    echo 'eval "$(pyenv init - bash)"' >> ~/.bashrc && \
    echo 'eval "$(pyenv virtualenv-init -)"' >> ~/.bashrc && \
    echo 'if [ -f "${WORKSPACE}/Pipfile" ]; then' >> ~/.bashrc && \
    echo '  PY_VER=$(awk -F\" '"'"'/python_version/ { print $2 }'"'"' "${WORKSPACE}/Pipfile")' >> ~/.bashrc && \
    echo '  if [ -n "${PY_VER}" ]; then' >> ~/.bashrc && \
    echo '    if ! pyenv versions --bare | grep -qx "${PY_VER}"; then' >> ~/.bashrc && \
    echo '      echo "[pyenv] Checking for Python ${PY_VER}..."' >> ~/.bashrc && \
    echo '      pyenv install --skip-existing "${PY_VER}"' >> ~/.bashrc && \
    echo '    fi' >> ~/.bashrc && \
    echo '    if [ "$(pyenv version-name)" != "${PY_VER}" ]; then' >> ~/.bashrc && \
    echo '      pyenv shell "${PY_VER}"' >> ~/.bashrc && \
    echo '    fi' >> ~/.bashrc && \
    echo '  fi' >> ~/.bashrc && \
    echo 'fi' >> ~/.bashrc
