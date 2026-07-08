FROM rust:1.89-bookworm AS builder

ARG LIBSYS_COVER_BASE=""
ENV LIBSYS_COVER_BASE=${LIBSYS_COVER_BASE}

RUN rustup target add wasm32-unknown-unknown \
  && cargo install dioxus-cli --locked

WORKDIR /app

COPY Cargo.toml ./
COPY Cargo.lock ./
COPY Dioxus.toml ./
COPY assets ./assets
COPY src ./src

RUN dx build --platform web --release

FROM nginx:1.27-alpine

COPY --from=builder /app/target/dx/frontend/release/web/public /usr/share/nginx/html
COPY nginx.conf /etc/nginx/conf.d/default.conf

EXPOSE 80

CMD ["nginx", "-g", "daemon off;"]
