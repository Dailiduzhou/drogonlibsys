---
name: dioxus-fullstack-dev
description: Build fullstack web applications with Dioxus using server functions, SSR, hydration, and API integration. Use when creating fullstack apps, implementing server-side rendering, or building APIs with client-server communication.
---

# Dioxus Fullstack Development

This skill helps you build comprehensive fullstack web applications using Dioxus with server-side rendering, server functions, and seamless client-server integration.

## When to use this skill

- Building fullstack web applications with SSR (Server-Side Rendering)
- Implementing server functions for backend logic
- Setting up client-server communication and data fetching
- Configuring hydration for optimal performance
- Building APIs with authentication and database integration
- Deploying fullstack Dioxus applications

## Fullstack Setup

### 1. Project Configuration

**Cargo.toml for Fullstack:**
```toml
[package]
name = "my-fullstack-app"
version = "0.1.0"
edition = "2021"

[dependencies]
dioxus = { version = "0.7", features = ["fullstack"] }
dioxus-router = "0.7"
serde = { version = "1.0", features = ["derive"] }
tokio = { version = "1.0", features = ["full"] }
sqlx = { version = "0.7", features = ["runtime-tokio-rustls", "postgres", "chrono", "uuid"] }
uuid = { version = "1.0", features = ["v4"] }
chrono = { version = "0.4", features = ["serde"] }
anyhow = "1.0"
tracing = "0.1"
tracing-subscriber = "0.3"

# Client-specific dependencies
[target.'cfg(not(target_arch = "wasm32"))'.dependencies]
axum = "0.7"
tower = "0.4"
tower-http = { version = "0.4", features = ["fs", "cors"] }

# WASM-specific dependencies
[target.'cfg(target_arch = "wasm32")'.dependencies]
wasm-bindgen = "0.2"
web-sys = "0.3"
```

**Dioxus.toml Configuration:**
```toml
[application]
name = "my-fullstack-app"
default_platform = "fullstack"

[web.app]
title = "My Fullstack App"
base_path = "."

[web.watcher]
watch_path = ["src", "assets"]
reload_html = true

[web.resource]
dev = true
style = ["./assets/main.css"]

[bundle]
identifier = "com.example.myapp"
publisher = "Example Publisher"
```

### 2. Application Structure

**src/main.rs (Server Entry Point):**
```rust
#![allow(non_snake_case)]

use dioxus::prelude::*;
use dioxus_fullstack::prelude::*;

mod app;
mod components;
mod server;
mod database;
mod auth;

#[tokio::main]
async fn main() {
    // Initialize tracing
    tracing_subscriber::init();
    
    // Initialize database
    let db_pool = database::init().await.expect("Failed to initialize database");
    
    // Configure the server
    let config = ServeConfig::builder()
        .assets_path("assets")
        .incremental(
            IncrementalRendererConfig::default()
                .static_dir("./static")
        )
        .build();
    
    // Launch the fullstack app
    LaunchBuilder::new()
        .with_cfg(config)
        .with_context(db_pool)
        .launch(app::App)
        .await
        .unwrap();
}
```

**src/app.rs (Root Component):**
```rust
#![allow(non_snake_case)]

use dioxus::prelude::*;
use dioxus_router::prelude::*;
use crate::components::*;

#[derive(Clone, Routable, Debug, PartialEq)]
enum Route {
    #[route("/")]
    Home {},
    
    #[route("/login")]
    Login {},
    
    #[route("/register")]
    Register {},
    
    #[layout(AuthLayout)]
        #[route("/dashboard")]
        Dashboard {},
        
        #[route("/profile")]
        Profile {},
        
        #[route("/posts")]
        Posts {},
        
        #[route("/posts/new")]
        CreatePost {},
        
        #[route("/posts/:id")]
        ViewPost { id: i32 },
    #[end_layout]
    
    #[route("/:..route")]
    PageNotFound { route: Vec<String> },
}

#[component]
pub fn App() -> Element {
    rsx! {
        document::Link { rel: "stylesheet", href: asset!("./assets/main.css") }
        Router::<Route> {}
    }
}

#[component]
fn Home() -> Element {
    let mut posts = use_resource(|| get_recent_posts());
    
    rsx! {
        div {
            class: "home",
            Header {}
            
            main {
                class: "container",
                h1 { "Welcome to My Fullstack App" }
                
                section {
                    class: "recent-posts",
                    h2 { "Recent Posts" }
                    
                    match posts() {
                        Some(Ok(posts_data)) => rsx! {
                            div {
                                class: "posts-grid",
                                for post in posts_data {
                                    PostCard { 
                                        key: "{post.id}",
                                        post: post.clone() 
                                    }
                                }
                            }
                        },
                        Some(Err(_)) => rsx! {
                            div { class: "error", "Failed to load posts" }
                        },
                        None => rsx! {
                            div { class: "loading", "Loading posts..." }
                        }
                    }
                }
            }
            
            Footer {}
        }
    }
}
```

### 3. Server Functions

**Define Server Functions:**
```rust
use dioxus::prelude::*;
use dioxus_fullstack::prelude::*;
use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
pub struct Post {
    pub id: i32,
    pub title: String,
    pub content: String,
    pub author_id: i32,
    pub created_at: chrono::DateTime<chrono::Utc>,
    pub updated_at: chrono::DateTime<chrono::Utc>,
}

#[derive(Serialize, Deserialize, Clone, Debug)]
pub struct CreatePostRequest {
    pub title: String,
    pub content: String,
}

#[server(GetRecentPosts)]
pub async fn get_recent_posts() -> Result<Vec<Post>, ServerFnError> {
    use crate::database::*;
    
    let db = get_db_pool()?;
    
    let posts = sqlx::query_as!(
        Post,
        "SELECT id, title, content, author_id, created_at, updated_at 
         FROM posts 
         ORDER BY created_at DESC 
         LIMIT 10"
    )
    .fetch_all(&db)
    .await
    .map_err(|e| ServerFnError::ServerError(e.to_string()))?;
    
    Ok(posts)
}

#[server(GetPost)]
pub async fn get_post(id: i32) -> Result<Option<Post>, ServerFnError> {
    use crate::database::*;
    
    let db = get_db_pool()?;
    
    let post = sqlx::query_as!(
        Post,
        "SELECT id, title, content, author_id, created_at, updated_at 
         FROM posts 
         WHERE id = $1",
        id
    )
    .fetch_optional(&db)
    .await
    .map_err(|e| ServerFnError::ServerError(e.to_string()))?;
    
    Ok(post)
}

#[server(CreatePost)]
pub async fn create_post(request: CreatePostRequest) -> Result<Post, ServerFnError> {
    use crate::database::*;
    use crate::auth::*;
    
    let db = get_db_pool()?;
    let user_id = get_current_user_id()?;
    
    let post = sqlx::query_as!(
        Post,
        "INSERT INTO posts (title, content, author_id, created_at, updated_at)
         VALUES ($1, $2, $3, NOW(), NOW())
         RETURNING id, title, content, author_id, created_at, updated_at",
        request.title,
        request.content,
        user_id
    )
    .fetch_one(&db)
    .await
    .map_err(|e| ServerFnError::ServerError(e.to_string()))?;
    
    Ok(post)
}

#[server(UpdatePost)]
pub async fn update_post(id: i32, request: CreatePostRequest) -> Result<Post, ServerFnError> {
    use crate::database::*;
    use crate::auth::*;
    
    let db = get_db_pool()?;
    let user_id = get_current_user_id()?;
    
    let post = sqlx::query_as!(
        Post,
        "UPDATE posts 
         SET title = $1, content = $2, updated_at = NOW()
         WHERE id = $3 AND author_id = $4
         RETURNING id, title, content, author_id, created_at, updated_at",
        request.title,
        request.content,
        id,
        user_id
    )
    .fetch_one(&db)
    .await
    .map_err(|e| ServerFnError::ServerError(e.to_string()))?;
    
    Ok(post)
}

#[server(DeletePost)]
pub async fn delete_post(id: i32) -> Result<(), ServerFnError> {
    use crate::database::*;
    use crate::auth::*;
    
    let db = get_db_pool()?;
    let user_id = get_current_user_id()?;
    
    let rows_affected = sqlx::query!(
        "DELETE FROM posts WHERE id = $1 AND author_id = $2",
        id,
        user_id
    )
    .execute(&db)
    .await
    .map_err(|e| ServerFnError::ServerError(e.to_string()))?
    .rows_affected();
    
    if rows_affected == 0 {
        return Err(ServerFnError::ServerError("Post not found or unauthorized".to_string()));
    }
    
    Ok(())
}
```

### 4. Authentication System

**src/auth.rs:**
```rust
use dioxus::prelude::*;
use dioxus_fullstack::prelude::*;
use serde::{Deserialize, Serialize};
use uuid::Uuid;

#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
pub struct User {
    pub id: i32,
    pub email: String,
    pub name: String,
    pub created_at: chrono::DateTime<chrono::Utc>,
}

#[derive(Serialize, Deserialize, Clone, Debug)]
pub struct LoginRequest {
    pub email: String,
    pub password: String,
}

#[derive(Serialize, Deserialize, Clone, Debug)]
pub struct RegisterRequest {
    pub email: String,
    pub name: String,
    pub password: String,
}

#[server(Login)]
pub async fn login(request: LoginRequest) -> Result<User, ServerFnError> {
    use crate::database::*;
    use argon2::{Argon2, PasswordHash, PasswordVerifier};
    
    let db = get_db_pool()?;
    
    // Get user and password hash from database
    let user_record = sqlx::query!(
        "SELECT id, email, name, password_hash, created_at FROM users WHERE email = $1",
        request.email
    )
    .fetch_optional(&db)
    .await
    .map_err(|e| ServerFnError::ServerError(e.to_string()))?;
    
    let user_record = user_record.ok_or_else(|| {
        ServerFnError::ServerError("Invalid credentials".to_string())
    })?;
    
    // Verify password
    let parsed_hash = PasswordHash::new(&user_record.password_hash)
        .map_err(|e| ServerFnError::ServerError(e.to_string()))?;
    
    Argon2::default()
        .verify_password(request.password.as_bytes(), &parsed_hash)
        .map_err(|_| ServerFnError::ServerError("Invalid credentials".to_string()))?;
    
    // Create session
    let session_token = Uuid::new_v4().to_string();
    
    sqlx::query!(
        "INSERT INTO sessions (user_id, token, expires_at)
         VALUES ($1, $2, NOW() + INTERVAL '30 days')",
        user_record.id,
        session_token
    )
    .execute(&db)
    .await
    .map_err(|e| ServerFnError::ServerError(e.to_string()))?;
    
    // Set session cookie
    let mut response = Response::new("".to_string());
    response.headers_mut().insert(
        "Set-Cookie",
        format!("session={}; Path=/; HttpOnly; SameSite=Strict; Max-Age=2592000", session_token)
            .parse()
            .unwrap(),
    );
    
    let user = User {
        id: user_record.id,
        email: user_record.email,
        name: user_record.name,
        created_at: user_record.created_at,
    };
    
    Ok(user)
}

#[server(Register)]
pub async fn register(request: RegisterRequest) -> Result<User, ServerFnError> {
    use crate::database::*;
    use argon2::{Argon2, PasswordHasher, password_hash::{SaltString, rand_core::OsRng}};
    
    let db = get_db_pool()?;
    
    // Check if user already exists
    let existing_user = sqlx::query!(
        "SELECT id FROM users WHERE email = $1",
        request.email
    )
    .fetch_optional(&db)
    .await
    .map_err(|e| ServerFnError::ServerError(e.to_string()))?;
    
    if existing_user.is_some() {
        return Err(ServerFnError::ServerError("User already exists".to_string()));
    }
    
    // Hash password
    let salt = SaltString::generate(&mut OsRng);
    let password_hash = Argon2::default()
        .hash_password(request.password.as_bytes(), &salt)
        .map_err(|e| ServerFnError::ServerError(e.to_string()))?
        .to_string();
    
    // Create user
    let user_record = sqlx::query!(
        "INSERT INTO users (email, name, password_hash, created_at)
         VALUES ($1, $2, $3, NOW())
         RETURNING id, email, name, created_at",
        request.email,
        request.name,
        password_hash
    )
    .fetch_one(&db)
    .await
    .map_err(|e| ServerFnError::ServerError(e.to_string()))?;
    
    let user = User {
        id: user_record.id,
        email: user_record.email,
        name: user_record.name,
        created_at: user_record.created_at,
    };
    
    Ok(user)
}

#[server(GetCurrentUser)]
pub async fn get_current_user() -> Result<Option<User>, ServerFnError> {
    use crate::database::*;
    
    let user_id = match get_current_user_id() {
        Ok(id) => id,
        Err(_) => return Ok(None),
    };
    
    let db = get_db_pool()?;
    
    let user_record = sqlx::query!(
        "SELECT id, email, name, created_at FROM users WHERE id = $1",
        user_id
    )
    .fetch_optional(&db)
    .await
    .map_err(|e| ServerFnError::ServerError(e.to_string()))?;
    
    let user = user_record.map(|record| User {
        id: record.id,
        email: record.email,
        name: record.name,
        created_at: record.created_at,
    });
    
    Ok(user)
}

#[server(Logout)]
pub async fn logout() -> Result<(), ServerFnError> {
    use crate::database::*;
    
    let db = get_db_pool()?;
    
    if let Ok(session_token) = get_session_token() {
        sqlx::query!(
            "DELETE FROM sessions WHERE token = $1",
            session_token
        )
        .execute(&db)
        .await
        .map_err(|e| ServerFnError::ServerError(e.to_string()))?;
    }
    
    // Clear session cookie
    let mut response = Response::new("".to_string());
    response.headers_mut().insert(
        "Set-Cookie",
        "session=; Path=/; HttpOnly; SameSite=Strict; Max-Age=0"
            .parse()
            .unwrap(),
    );
    
    Ok(())
}

// Helper functions for server context
pub fn get_current_user_id() -> Result<i32, ServerFnError> {
    // Implementation would extract user ID from session
    todo!("Implement session-based user ID extraction")
}

pub fn get_session_token() -> Result<String, ServerFnError> {
    // Implementation would extract session token from cookies
    todo!("Implement session token extraction")
}
```

### 5. Client Components with Server Integration

**src/components/login_form.rs:**
```rust
use dioxus::prelude::*;
use crate::auth::*;

#[component]
pub fn LoginForm() -> Element {
    let mut email = use_signal(|| String::new());
    let mut password = use_signal(|| String::new());
    let mut is_loading = use_signal(|| false);
    let mut error_message = use_signal(|| None::<String>);
    let navigator = use_navigator();
    
    let handle_submit = move |_| {
        is_loading.set(true);
        error_message.set(None);
        
        let email_val = email();
        let password_val = password();
        
        spawn(async move {
            match login(LoginRequest {
                email: email_val,
                password: password_val,
            }).await {
                Ok(_user) => {
                    navigator.push(Route::Dashboard {});
                }
                Err(e) => {
                    error_message.set(Some(e.to_string()));
                    is_loading.set(false);
                }
            }
        });
    };
    
    rsx! {
        div {
            class: "login-form-container",
            
            form {
                class: "login-form",
                onsubmit: handle_submit,
                
                h2 { "Login" }
                
                if let Some(error) = error_message() {
                    div {
                        class: "error-message",
                        "{error}"
                    }
                }
                
                div {
                    class: "form-group",
                    label { "Email:" }
                    input {
                        r#type: "email",
                        required: true,
                        value: "{email}",
                        oninput: move |evt| email.set(evt.value()),
                        disabled: is_loading()
                    }
                }
                
                div {
                    class: "form-group",
                    label { "Password:" }
                    input {
                        r#type: "password",
                        required: true,
                        value: "{password}",
                        oninput: move |evt| password.set(evt.value()),
                        disabled: is_loading()
                    }
                }
                
                button {
                    r#type: "submit",
                    class: "submit-btn",
                    disabled: is_loading(),
                    if is_loading() {
                        "Logging in..."
                    } else {
                        "Login"
                    }
                }
                
                div {
                    class: "form-footer",
                    p {
                        "Don't have an account? "
                        Link { to: Route::Register {}, "Register here" }
                    }
                }
            }
        }
    }
}
```

**src/components/post_form.rs:**
```rust
use dioxus::prelude::*;
use crate::server::*;

#[component]
pub fn CreatePostForm() -> Element {
    let mut title = use_signal(|| String::new());
    let mut content = use_signal(|| String::new());
    let mut is_submitting = use_signal(|| false);
    let mut success_message = use_signal(|| None::<String>);
    let mut error_message = use_signal(|| None::<String>);
    let navigator = use_navigator();
    
    let handle_submit = move |_| {
        if title().trim().is_empty() || content().trim().is_empty() {
            error_message.set(Some("Title and content are required".to_string()));
            return;
        }
        
        is_submitting.set(true);
        error_message.set(None);
        success_message.set(None);
        
        let title_val = title();
        let content_val = content();
        
        spawn(async move {
            match create_post(CreatePostRequest {
                title: title_val,
                content: content_val,
            }).await {
                Ok(post) => {
                    success_message.set(Some("Post created successfully!".to_string()));
                    // Navigate to the new post after a brief delay
                    spawn(async move {
                        tokio::time::sleep(std::time::Duration::from_secs(2)).await;
                        navigator.push(Route::ViewPost { id: post.id });
                    });
                }
                Err(e) => {
                    error_message.set(Some(e.to_string()));
                    is_submitting.set(false);
                }
            }
        });
    };
    
    rsx! {
        div {
            class: "create-post-container",
            
            h1 { "Create New Post" }
            
            if let Some(success) = success_message() {
                div {
                    class: "success-message",
                    "{success}"
                }
            }
            
            if let Some(error) = error_message() {
                div {
                    class: "error-message",
                    "{error}"
                }
            }
            
            form {
                class: "post-form",
                onsubmit: handle_submit,
                
                div {
                    class: "form-group",
                    label { "Title:" }
                    input {
                        r#type: "text",
                        required: true,
                        value: "{title}",
                        oninput: move |evt| title.set(evt.value()),
                        disabled: is_submitting(),
                        placeholder: "Enter post title..."
                    }
                }
                
                div {
                    class: "form-group",
                    label { "Content:" }
                    textarea {
                        required: true,
                        value: "{content}",
                        oninput: move |evt| content.set(evt.value()),
                        disabled: is_submitting(),
                        placeholder: "Write your post content here...",
                        rows: "10"
                    }
                }
                
                div {
                    class: "form-actions",
                    button {
                        r#type: "button",
                        class: "cancel-btn",
                        onclick: move |_| navigator.push(Route::Posts {}),
                        "Cancel"
                    }
                    
                    button {
                        r#type: "submit",
                        class: "submit-btn",
                        disabled: is_submitting(),
                        if is_submitting() {
                            "Creating..."
                        } else {
                            "Create Post"
                        }
                    }
                }
            }
        }
    }
}
```

### 6. Database Setup

**src/database.rs:**
```rust
use sqlx::{Pool, Postgres};
use anyhow::Result;

pub type DbPool = Pool<Postgres>;

pub async fn init() -> Result<DbPool> {
    let database_url = std::env::var("DATABASE_URL")
        .expect("DATABASE_URL must be set");
    
    let pool = sqlx::PgPool::connect(&database_url).await?;
    
    // Run migrations
    sqlx::migrate!("./migrations").run(&pool).await?;
    
    Ok(pool)
}

pub fn get_db_pool() -> Result<DbPool, dioxus_fullstack::ServerFnError> {
    use dioxus_fullstack::prelude::*;
    
    extract()
        .ok_or_else(|| ServerFnError::ServerError("Database pool not found".to_string()))
}
```

## Advanced Features

### 1. Real-time Updates with WebSockets

```rust
#[server(SubscribeToUpdates)]
pub async fn subscribe_to_updates() -> Result<(), ServerFnError> {
    // WebSocket implementation for real-time updates
    todo!("Implement WebSocket subscription")
}

#[component]
fn RealTimePostList() -> Element {
    let mut posts = use_signal(|| Vec::<Post>::new());
    
    // Subscribe to real-time updates
    use_effect(|| {
        spawn(async move {
            subscribe_to_updates().await.ok();
        });
    });
    
    rsx! {
        div {
            class: "realtime-posts",
            for post in posts() {
                PostCard { 
                    key: "{post.id}",
                    post: post.clone() 
                }
            }
        }
    }
}
```

### 2. File Upload Handling

```rust
#[server(UploadFile)]
pub async fn upload_file(file_data: Vec<u8>, filename: String) -> Result<String, ServerFnError> {
    use std::path::Path;
    
    let upload_dir = Path::new("uploads");
    let file_path = upload_dir.join(&filename);
    
    tokio::fs::create_dir_all(upload_dir).await
        .map_err(|e| ServerFnError::ServerError(e.to_string()))?;
    
    tokio::fs::write(&file_path, file_data).await
        .map_err(|e| ServerFnError::ServerError(e.to_string()))?;
    
    Ok(format!("/uploads/{}", filename))
}

#[component]
fn FileUpload() -> Element {
    let mut selected_file = use_signal(|| None::<web_sys::File>);
    let mut upload_progress = use_signal(|| 0);
    let mut is_uploading = use_signal(|| false);
    
    let handle_upload = move |_| {
        if let Some(file) = selected_file() {
            is_uploading.set(true);
            
            spawn(async move {
                let file_data = read_file_as_bytes(&file).await;
                match upload_file(file_data, file.name()).await {
                    Ok(url) => {
                        // Handle successful upload
                        println!("File uploaded to: {}", url);
                    }
                    Err(e) => {
                        println!("Upload failed: {}", e);
                    }
                }
                is_uploading.set(false);
            });
        }
    };
    
    rsx! {
        div {
            class: "file-upload",
            input {
                r#type: "file",
                onchange: move |evt| {
                    if let Some(files) = evt.files() {
                        if let Some(file) = files.get(0) {
                            selected_file.set(Some(file));
                        }
                    }
                }
            }
            
            button {
                onclick: handle_upload,
                disabled: selected_file().is_none() || is_uploading(),
                if is_uploading() {
                    "Uploading..."
                } else {
                    "Upload File"
                }
            }
        }
    }
}
```

## Deployment

### 1. Build Configuration

```bash
# Build for production
dx build --release --platform fullstack

# Build client and server separately
dx build --release --platform web
dx build --release --platform server
```

### 2. Docker Setup

**Dockerfile:**
```dockerfile
FROM rust:1.75 as builder

WORKDIR /app
COPY . .
RUN dx build --release --platform server

FROM debian:bookworm-slim
RUN apt-get update && apt-get install -y \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /app/dist /app/dist
COPY --from=builder /app/target/release/my-fullstack-app /app/

WORKDIR /app
EXPOSE 8080

CMD ["./my-fullstack-app"]
```

This skill provides comprehensive patterns for building production-ready fullstack applications with Dioxus, including server functions, authentication, database integration, and deployment strategies.