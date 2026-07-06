---
name: dioxus-routing
description: Implement client-side routing in Dioxus applications using dioxus-router. Use when building multi-page applications, setting up navigation, implementing nested routes, or handling route parameters and navigation guards.
---

# Dioxus Routing

This skill helps you implement robust client-side routing in Dioxus applications using the `dioxus-router` crate.

## When to use this skill

- Setting up client-side routing for single-page applications (SPAs)
- Creating multi-page Dioxus applications with navigation
- Implementing nested routes and route hierarchies
- Handling route parameters, query strings, and navigation state
- Setting up route guards and authentication-based routing
- Managing browser history and navigation programmatically

## Router Setup

### 1. Add Router Dependencies

Add the router dependency to your `Cargo.toml`:

```toml
[dependencies]
dioxus = { version = "0.7", features = ["web", "router"] }
dioxus-router = "0.7"
```

### 2. Define Route Structure

Create a route enum that represents your application's pages:

```rust
use dioxus::prelude::*;
use dioxus_router::prelude::*;

#[derive(Clone, Routable, Debug, PartialEq)]
enum Route {
    #[route("/")]
    Home {},
    
    #[route("/about")]
    About {},
    
    #[route("/blog")]
    Blog {},
    
    #[route("/blog/:id")]
    BlogPost { id: usize },
    
    #[route("/user/:user_id")]
    UserProfile { user_id: String },
    
    #[route("/user/:user_id/settings")]
    UserSettings { user_id: String },
    
    #[route("/products?:category&:sort")]
    Products { 
        category: Option<String>, 
        sort: Option<String> 
    },
    
    // Catch-all route for 404 pages
    #[route("/:..route")]
    PageNotFound { route: Vec<String> },
}
```

### 3. Basic Router Implementation

Set up the main application with router:

```rust
#![allow(non_snake_case)]
use dioxus::prelude::*;
use dioxus_router::prelude::*;

fn main() {
    dioxus::launch(App);
}

#[component]
fn App() -> Element {
    rsx! {
        Router::<Route> {}
    }
}

#[component]
fn Home() -> Element {
    rsx! {
        div {
            class: "home-page",
            h1 { "Welcome to Home Page" }
            nav {
                Link { to: Route::About {}, "About" }
                Link { to: Route::Blog {}, "Blog" }
            }
        }
    }
}

#[component]
fn About() -> Element {
    rsx! {
        div {
            class: "about-page",
            h1 { "About Us" }
            p { "This is the about page" }
            Link { to: Route::Home {}, "Back to Home" }
        }
    }
}
```

## Advanced Routing Patterns

### 1. Nested Routes with Layout

```rust
#[derive(Clone, Routable, Debug, PartialEq)]
enum Route {
    #[route("/")]
    Home {},
    
    #[layout(DashboardLayout)]
        #[route("/dashboard")]
        Dashboard {},
        
        #[route("/dashboard/analytics")]
        Analytics {},
        
        #[route("/dashboard/settings")]
        Settings {},
    #[end_layout]
    
    #[route("/:..route")]
    PageNotFound { route: Vec<String> },
}

#[component]
fn DashboardLayout() -> Element {
    rsx! {
        div {
            class: "dashboard-layout",
            header {
                class: "dashboard-header",
                h1 { "Dashboard" }
                nav {
                    class: "dashboard-nav",
                    Link { 
                        to: Route::Dashboard {}, 
                        class: "nav-link",
                        "Overview" 
                    }
                    Link { 
                        to: Route::Analytics {}, 
                        class: "nav-link",
                        "Analytics" 
                    }
                    Link { 
                        to: Route::Settings {}, 
                        class: "nav-link",
                        "Settings" 
                    }
                }
            }
            main {
                class: "dashboard-content",
                // This is where nested route components render
                Outlet::<Route> {}
            }
        }
    }
}
```

### 2. Route Parameters and Query Handling

```rust
#[component]
fn BlogPost(id: usize) -> Element {
    let mut post_data = use_signal(|| None::<BlogPostData>);
    let navigator = use_navigator();
    
    // Load post data based on ID
    use_effect(move || {
        spawn(async move {
            match fetch_blog_post(id).await {
                Ok(post) => post_data.set(Some(post)),
                Err(_) => navigator.push(Route::PageNotFound { 
                    route: vec!["blog".to_string(), id.to_string()] 
                }),
            }
        });
    });
    
    match post_data() {
        Some(post) => rsx! {
            article {
                class: "blog-post",
                header {
                    h1 { "{post.title}" }
                    p { class: "meta", "Published: {post.date}" }
                }
                div {
                    class: "content",
                    dangerous_inner_html: "{post.content}"
                }
                nav {
                    class: "post-navigation",
                    Link { to: Route::Blog {}, "← Back to Blog" }
                    if let Some(next_id) = post.next_id {
                        Link { 
                            to: Route::BlogPost { id: next_id }, 
                            "Next Post →" 
                        }
                    }
                }
            }
        },
        None => rsx! {
            div { class: "loading", "Loading post..." }
        }
    }
}

#[component]
fn Products(category: Option<String>, sort: Option<String>) -> Element {
    let mut products = use_signal(|| Vec::<Product>::new());
    let navigator = use_navigator();
    
    // Update URL when filters change
    let update_filters = use_callback(move |new_category: Option<String>, new_sort: Option<String>| {
        navigator.push(Route::Products { 
            category: new_category, 
            sort: new_sort 
        });
    });
    
    rsx! {
        div {
            class: "products-page",
            h1 { "Products" }
            
            div {
                class: "filters",
                select {
                    onchange: move |evt| {
                        let new_category = if evt.value().is_empty() { 
                            None 
                        } else { 
                            Some(evt.value()) 
                        };
                        update_filters.call((new_category, sort.clone()));
                    },
                    option { value: "", "All Categories" }
                    option { 
                        value: "electronics", 
                        selected: category.as_deref() == Some("electronics"),
                        "Electronics" 
                    }
                    option { 
                        value: "clothing", 
                        selected: category.as_deref() == Some("clothing"),
                        "Clothing" 
                    }
                }
                
                select {
                    onchange: move |evt| {
                        let new_sort = if evt.value().is_empty() { 
                            None 
                        } else { 
                            Some(evt.value()) 
                        };
                        update_filters.call((category.clone(), new_sort));
                    },
                    option { value: "", "Default Sort" }
                    option { 
                        value: "price_asc", 
                        selected: sort.as_deref() == Some("price_asc"),
                        "Price: Low to High" 
                    }
                    option { 
                        value: "price_desc", 
                        selected: sort.as_deref() == Some("price_desc"),
                        "Price: High to Low" 
                    }
                }
            }
            
            div {
                class: "products-grid",
                for product in products() {
                    ProductCard { product: product.clone() }
                }
            }
        }
    }
}
```

### 3. Programmatic Navigation

```rust
#[component]
fn LoginForm() -> Element {
    let mut username = use_signal(|| String::new());
    let mut password = use_signal(|| String::new());
    let mut is_loading = use_signal(|| false);
    let navigator = use_navigator();
    
    let handle_login = move |_| {
        if !username().is_empty() && !password().is_empty() {
            is_loading.set(true);
            
            let nav = navigator.clone();
            let user = username();
            let pass = password();
            
            spawn(async move {
                match authenticate(&user, &pass).await {
                    Ok(user_data) => {
                        // Store user session
                        store_user_session(user_data).await;
                        
                        // Navigate to dashboard
                        nav.push(Route::Dashboard {});
                    }
                    Err(e) => {
                        // Handle error (you might want to show an error message)
                        eprintln!("Login failed: {}", e);
                        is_loading.set(false);
                    }
                }
            });
        }
    };
    
    rsx! {
        form {
            class: "login-form",
            onsubmit: handle_login,
            
            input {
                r#type: "text",
                placeholder: "Username",
                value: "{username}",
                oninput: move |evt| username.set(evt.value())
            }
            
            input {
                r#type: "password",
                placeholder: "Password",
                value: "{password}",
                oninput: move |evt| password.set(evt.value())
            }
            
            button {
                r#type: "submit",
                disabled: is_loading(),
                if is_loading() { "Logging in..." } else { "Login" }
            }
        }
    }
}
```

### 4. Route Guards and Protection

```rust
#[component]
fn ProtectedRoute(children: Element) -> Element {
    let auth_state = use_context::<AuthState>();
    let navigator = use_navigator();
    
    use_effect(move || {
        if !auth_state.is_authenticated() {
            navigator.push(Route::Home {});
        }
    });
    
    if auth_state.is_authenticated() {
        rsx! { {children} }
    } else {
        rsx! {
            div {
                class: "access-denied",
                h2 { "Access Denied" }
                p { "Please log in to access this page." }
                Link { to: Route::Home {}, "Go to Home" }
            }
        }
    }
}

// Usage in route definition
#[component]
fn Dashboard() -> Element {
    rsx! {
        ProtectedRoute {
            div {
                class: "dashboard",
                h1 { "Dashboard" }
                p { "Welcome to your dashboard!" }
            }
        }
    }
}
```

### 5. Active Link Styling

```rust
#[component]
fn Navigation() -> Element {
    let current_route = use_route::<Route>();
    
    rsx! {
        nav {
            class: "main-navigation",
            Link {
                to: Route::Home {},
                class: if matches!(current_route, Route::Home {}) {
                    "nav-link active"
                } else {
                    "nav-link"
                },
                "Home"
            }
            Link {
                to: Route::About {},
                class: if matches!(current_route, Route::About {}) {
                    "nav-link active"
                } else {
                    "nav-link"
                },
                "About"
            }
            Link {
                to: Route::Blog {},
                class: if matches!(current_route, Route::Blog {} | Route::BlogPost { .. }) {
                    "nav-link active"
                } else {
                    "nav-link"
                },
                "Blog"
            }
        }
    }
}
```

## Advanced Features

### 1. Route Transitions

```rust
#[component]
fn App() -> Element {
    rsx! {
        div {
            class: "app",
            style: "transition: all 0.3s ease-in-out;",
            Router::<Route> {}
        }
    }
}
```

### 2. Lazy Loading Routes

```rust
#[component]
fn LazyRoute() -> Element {
    let mut component = use_signal(|| None::<Element>);
    
    use_effect(|| {
        spawn(async move {
            // Simulate dynamic import
            let loaded_component = load_component().await;
            component.set(Some(loaded_component));
        });
    });
    
    match component() {
        Some(comp) => comp,
        None => rsx! {
            div { class: "loading", "Loading..." }
        }
    }
}
```

### 3. Error Boundaries for Routes

```rust
#[component]
fn PageNotFound(route: Vec<String>) -> Element {
    let path = route.join("/");
    
    rsx! {
        div {
            class: "not-found",
            h1 { "404 - Page Not Found" }
            p { "The page '/{path}' could not be found." }
            nav {
                Link { to: Route::Home {}, "Go Home" }
                button {
                    onclick: move |_| {
                        web_sys::window()
                            .unwrap()
                            .history()
                            .unwrap()
                            .back()
                            .ok();
                    },
                    "Go Back"
                }
            }
        }
    }
}
```

## Best Practices

### 1. Route Organization
- Keep routes organized in a logical hierarchy
- Use consistent naming conventions
- Group related routes with layouts
- Keep route parameters type-safe

### 2. Performance Optimization
- Use lazy loading for heavy route components
- Implement proper loading states
- Cache route data when appropriate
- Minimize route component re-renders

### 3. User Experience
- Provide clear navigation indicators
- Implement proper loading and error states
- Use browser back/forward button correctly
- Handle deep linking and bookmarking

### 4. SEO Considerations
- Use meaningful URLs
- Implement proper meta tags per route
- Consider server-side rendering for better SEO
- Provide fallback routes for better crawling

## Common Patterns

### Breadcrumb Navigation
```rust
#[component]
fn Breadcrumbs() -> Element {
    let route = use_route::<Route>();
    let breadcrumbs = generate_breadcrumbs(&route);
    
    rsx! {
        nav {
            class: "breadcrumbs",
            for (i, crumb) in breadcrumbs.iter().enumerate() {
                if i > 0 {
                    span { class: "separator", ">" }
                }
                if let Some(route) = &crumb.route {
                    Link { to: route.clone(), "{crumb.label}" }
                } else {
                    span { class: "current", "{crumb.label}" }
                }
            }
        }
    }
}
```

This skill provides comprehensive routing capabilities for building sophisticated single-page applications with Dioxus.