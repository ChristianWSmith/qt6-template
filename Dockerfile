# Toolchain-only image (AUD-108): compiler/cmake/pipenv/build deps, no system
# Qt — Qt still comes from the local Qt/ tree via scripts/install-qt.sh.
# Base selected so the default toolchain matches conan/profiles/linux
# compiler.version=15: Ubuntu 26.04 LTS ships GCC 15 as the default
# (build-essential → gcc/g++ 15). gcc-15/g++-15 are installed explicitly so
# the match is intentional, not incidental.
FROM ubuntu:26.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    gcc-15 \
    g++-15 \
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
    make \
    libssl-dev \
    zlib1g-dev \
    libbz2-dev \
    libreadline-dev \
    libsqlite3-dev \
    llvm \
    libncurses-dev \
    xz-utils \
    tk-dev \
    libxml2-dev \
    libxmlsec1-dev \
    libffi-dev \
    liblzma-dev \
    sudo \
    locales \
    locales-all \
    autotrace \
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
    libxcb-util-dev \
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
    lldb \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Make the profile compiler the default alternatives target so plain
# gcc/g++ (and Conan detection) see major 15, matching conan/profiles/linux.
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-15 150 \
        --slave /usr/bin/g++ g++ /usr/bin/g++-15 \
        --slave /usr/bin/gcov gcov /usr/bin/gcov-15 \
    && gcc --version | head -1 \
    && g++ --version | head -1

# Ubuntu 24.04+ marks system Python as externally-managed (PEP 668).
RUN pip3 install --no-cache-dir --upgrade --break-system-packages pip pipenv

RUN groupadd -g 1000 devgroup && \
    useradd -m -u 1000 -g devgroup devuser && \
    echo 'devuser ALL=(ALL) NOPASSWD:ALL' > /etc/sudoers && \
    chmod 0440 /etc/sudoers

USER devuser

RUN curl https://pyenv.run | bash

RUN curl -fsSL https://starship.rs/install.sh | sh -s -- --yes

RUN mkdir -p "${HOME}/.config" && \
    cat > "${HOME}/.config/starship.toml" <<'EOF'
[directory]
style = "cyan"
truncate_to_repo = true
EOF

RUN cat > "${HOME}/.bashrc" <<'EOF'
eval "$(starship init bash)"

if [ ! -d "${WORKSPACE}/.pyenv" ]; then
  mv "${HOME}/.pyenv" "${WORKSPACE}/.pyenv"
fi

export PYENV_ROOT="${WORKSPACE}/.pyenv"
export PATH="${PYENV_ROOT}/bin:${WORKSPACE}/scripts:${PATH}"
eval "$(pyenv init - bash)"
eval "$(pyenv virtualenv-init -)"

if [ -f "${WORKSPACE}/Pipfile" ]; then
  PY_VER=$(awk -F\" '/python_version/ { print $2 }' "${WORKSPACE}/Pipfile")
  if [ -n "${PY_VER}" ]; then
    if ! pyenv versions --bare | grep -qx "${PY_VER}"; then
      echo "[pyenv] Checking for Python ${PY_VER}..."
      pyenv install --skip-existing "${PY_VER}"
    fi
    if [ "$(pyenv version-name)" != "${PY_VER}" ]; then
      pyenv shell "${PY_VER}"
    fi
  fi
fi

lldb-debug() {
  lldb-server platform --server --listen "0.0.0.0:${LLDB_PORT}" & run.sh
}
EOF
