FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV LIBSYS_CONFIG_PATH=config/config.docker.json
ENV XMAKE_ROOT=y
ENV PKG_CONFIG_PATH=/usr/local/lib/pkgconfig

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
  libjsoncpp-dev \
  libpq-dev \
  libssl-dev \
  uuid-dev \
  libyaml-cpp-dev \
  ninja-build \
  pkg-config \
  zlib1g-dev \
  && rm -rf /var/lib/apt/lists/*

RUN git clone --recursive --depth 1 \
  https://github.com/drogonframework/drogon.git /tmp/drogon \
  && cd /tmp/drogon \
  && git submodule update --init \
  && mkdir build && cd build \
  && cmake .. -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr/local \
  -DBUILD_POSTGRESQL=ON \
  -DBUILD_MYSQL=OFF \
  -DBUILD_SQLITE=OFF \
  -DBUILD_REDIS=OFF \
  -DBUILD_TESTING=OFF \
  -DBUILD_EXAMPLES=OFF \
  -DBUILD_CTL=OFF \
  && ninja \
  && ninja install \
  && rm -rf /tmp/drogon

RUN curl -fsSL https://xmake.io/shget.text | bash \
  && . ~/.xmake/profile \
  && xmake --version

# 纯 C 模块 (libxcrypt bcrypt) 与 sqlite3 链接所需的 dev 包
# 单独一层, 不影响 drogon 编译缓存
RUN apt-get update && apt-get install -y --no-install-recommends \
  libcrypt-dev \
  libsqlite3-dev \
  && rm -rf /var/lib/apt/lists/*

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
