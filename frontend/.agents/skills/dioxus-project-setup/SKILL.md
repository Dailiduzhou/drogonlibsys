---
name: dioxus-project-setup
description: Set up a new Dioxus project with proper structure, configuration, and boilerplate code. Use when creating new Dioxus apps, configuring build systems, or initializing project templates.
---

# Dioxus Project Setup

This skill helps you create and configure new Dioxus projects with the proper structure, dependencies, and boilerplate code.

## When to use this skill

- Creating a new Dioxus application from scratch
- Setting up a Dioxus project with specific platform targets (web, desktop, mobile, fullstack)
- Configuring Dioxus CLI and build tools
- Adding Dioxus to an existing Rust project
- Setting up project structure with components, assets, and routing

## Setup Steps

### 1. Install Dioxus CLI

First, ensure the Dioxus CLI tool `dx` is installed:

```bash
# Install cargo-binstall if not already installed
cargo install cargo-binstall

# Install the precompiled dx tool
cargo binstall dioxus-cli
```

### 2. Create New Project

Create a new Dioxus project using the CLI:

```bash
# Interactive project creation
dx new my-app

# Or specify template directly
dx new my-app --template web
dx new my-app --template desktop
dx new my-app --template mobile
dx new my-app --template fullstack
```

### 3. Project Structure

A typical Dioxus project should have this structure:

```
my-app/
├── Cargo.toml
├── Dioxus.toml           # Dioxus configuration
├── src/
│   ├── main.rs           # Application entry point
│   ├── app.rs            # Root component
│   └── components/       # Reusable components
│       └── mod.rs
├── assets/               # Static assets
│   ├── main.css
│   └── favicon.ico
└── public/              # Public web assets
    └── tailwind.css
```

### 4. Configure Cargo.toml

Add appropriate Dioxus dependencies based on target platform:

**Web Application:**
```toml
[dependencies]
dioxus = { version = "0.7", features = ["web"] }
dioxus-logger = "0.7"
```

**Desktop Application:**
```toml
[dependencies]
dioxus = { version = "0.7", features = ["desktop"] }
dioxus-logger = "0.7"
```

**Fullstack Application:**
```toml
[dependencies]
dioxus = { version = "0.7", features = ["fullstack"] }
dioxus-router = "0.7"
serde = { version = "1.0", features = ["derive"] }
tokio = { version = "1.0", features = ["full"] }
```

**Mobile Application:**
```toml
[dependencies]
dioxus = { version = "0.7", features = ["mobile"] }
dioxus-logger = "0.7"
```

### 5. Configure Dioxus.toml

Create a `Dioxus.toml` configuration file:

```toml
[application]
name = "my-app"
default_platform = "web"

[web.app]
title = "My Dioxus App"
base_path = "."

[web.watcher]
watch_path = ["src", "assets"]
reload_html = true
index_on_404 = true

[web.resource]
dev = true
style = ["./assets/main.css"]
script = []

[bundle]
identifier = "com.example.myapp"
publisher = "Example Publisher"
icon = ["./assets/favicon.ico"]
resources = ["./assets/"]
copyright = "Copyright (c) Example Publisher 2024"
category = "DeveloperTool"
short_description = "My Dioxus Application"
long_description = """
A cross-platform application built with Dioxus.
"""
```

### 6. Create Main Application Structure

**src/main.rs:**
```rust
#![allow(non_snake_case)]
use dioxus::prelude::*;
use dioxus_logger::tracing::{info, Level};

mod app;
mod components;

fn main() {
    // Init logger
    dioxus_logger::init(Level::INFO).expect("failed to init logger");
    info!("starting app");
    
    dioxus::launch(app::App);
}
```

**src/app.rs:**
```rust
#![allow(non_snake_case)]
use dioxus::prelude::*;
use crate::components::*;

#[component]
pub fn App() -> Element {
    rsx! {
        div {
            class: "app-container",
            Header { title: "My Dioxus App" }
            main {
                class: "main-content",
                "Welcome to Dioxus!"
            }
            Footer {}
        }
    }
}
```

### 7. Development Commands

After setup, use these commands for development:

```bash
# Serve for web development
dx serve --web

# Serve for desktop development
dx serve --desktop

# Build for production
dx build --web --release

# Format RSX code
dx fmt

# Check for issues
dx check
```

## Additional Configurations

### Hot Reload Setup

Ensure hot reload works by adding to `Cargo.toml`:

```toml
[dependencies]
dioxus = { version = "0.7", features = ["web", "hot-reload"] }
```

### Styling Integration

For Tailwind CSS integration, add to `Dioxus.toml`:

```toml
[web.resource]
style = ["./public/tailwind.css", "./assets/main.css"]
```

### Router Setup (for multi-page apps)

Add router dependency and setup:

```toml
[dependencies]
dioxus-router = "0.7"
```

## Troubleshooting

### Common Issues:

1. **dx command not found**: Ensure dioxus-cli is properly installed
2. **Hot reload not working**: Check that `hot-reload` feature is enabled
3. **Assets not loading**: Verify asset paths in `Dioxus.toml`
4. **Build failures**: Check Rust version compatibility (Dioxus requires Rust 1.70+)

### Platform-Specific Notes:

- **Web**: Requires `wasm-pack` for WASM compilation
- **Desktop**: Uses Tauri/Wry backend, requires WebView2 on Windows
- **Mobile**: Requires platform-specific SDKs (Android SDK, Xcode)
- **Fullstack**: Requires both server and client feature configurations

## Next Steps

After project setup:

1. Define your component structure in `src/components/`
2. Set up routing if needed with `dioxus-router`
3. Configure state management with `dioxus-signals`
4. Add styling with CSS frameworks or inline styles
5. Set up build and deployment pipelines

The project is now ready for Dioxus development!