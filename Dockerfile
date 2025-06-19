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
    libvulkan1 \
    vulkan-tools \
    vulkan-validationlayers \
    libvulkan-dev \
    lldb \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

RUN pip3 install --no-cache-dir --upgrade pip pipenv

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
EOF
