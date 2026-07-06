FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV LIBSYS_CONFIG_PATH=config/config.docker.json

RUN apt-get update && apt-get install -y \
    build-essential \
    ca-certificates \
    curl \
    git \
    libcurl4-openssl-dev \
    libhiredis-dev \
    libpqxx-dev \
    libssl-dev \
    pkg-config \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

RUN curl -fsSL https://xmake.io/shget.text | bash \
    && ln -s /root/.local/bin/xmake /usr/local/bin/xmake

WORKDIR /app

COPY . .

RUN mkdir -p logs \
    && xmake f -m release \
    && xmake build drogonlibsys

EXPOSE 8080

CMD ["./build/bin/drogonlibsys"]
