FROM rust:1.89-bookworm AS builder

RUN rustup target add wasm32-unknown-unknown \
    && cargo install dioxus-cli --locked

WORKDIR /app

COPY Cargo.toml ./
COPY src ./src

RUN dx build --platform web --release

FROM nginx:1.27-alpine

COPY --from=builder /app/dist /usr/share/nginx/html

EXPOSE 80

CMD ["nginx", "-g", "daemon off;"]
