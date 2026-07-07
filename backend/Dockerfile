FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV LIBSYS_CONFIG_PATH=config/config.docker.json
ENV XMAKE_ROOT=y

ARG XMAKE_PKG_MIRROR

SHELL ["/bin/bash", "-lc"]

RUN apt-get update && apt-get install -y --no-install-recommends \
  build-essential \
  ca-certificates \
  cmake \
  curl \
  git \
  libcurl4-openssl-dev \
  libhiredis-dev \
  libssl-dev \
  libyaml-cpp-dev \
  ninja-build \
  pkg-config \
  zlib1g-dev \
  && rm -rf /var/lib/apt/lists/*

RUN curl -fsSL https://xmake.io/shget.text | bash \
  && . ~/.xmake/profile \
  && xmake --version

WORKDIR /app

COPY . .

RUN --mount=type=cache,target=/root/.xmake/cache \
    mkdir -p logs \
  && . ~/.xmake/profile \
  && if [ -n "$XMAKE_PKG_MIRROR" ]; then xmake g --pkg_mirror="$XMAKE_PKG_MIRROR" -y; fi \
  && xmake f -m release -y \
  && xmake build drogonlibsys

EXPOSE 8080

CMD ["./build/bin/drogonlibsys"]
