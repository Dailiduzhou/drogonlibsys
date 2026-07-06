---
name: dioxus-component-dev
description: Create, design, and optimize Dioxus components with proper patterns, hooks, props, and RSX syntax. Use when building UI components, managing component state, or implementing component composition patterns.
---

# Dioxus Component Development

This skill helps you create, design, and optimize Dioxus components using best practices, proper patterns, and efficient RSX syntax.

## When to use this skill

- Creating new Dioxus components with proper structure
- Implementing component props and state management
- Writing RSX markup with proper syntax and patterns
- Optimizing component performance and reactivity
- Implementing component composition and reusability patterns
- Debugging component lifecycle and rendering issues

## Component Development Patterns

### 1. Basic Component Structure

**Simple Functional Component:**
```rust
#![allow(non_snake_case)]
use dioxus::prelude::*;

#[component]
fn MyComponent() -> Element {
    rsx! {
        div {
            class: "my-component",
            "Hello from MyComponent!"
        }
    }
}
```

**Component with Props:**
```rust
#[component]
fn UserCard(name: String, email: String, avatar_url: Option<String>) -> Element {
    rsx! {
        div {
            class: "user-card",
            if let Some(avatar) = avatar_url {
                img {
                    src: "{avatar}",
                    alt: "Avatar for {name}",
                    class: "avatar"
                }
            }
            div {
                class: "user-info",
                h3 { "{name}" }
                p { "{email}" }
            }
        }
    }
}
```

### 2. Component State Management

**Using Signals for Local State:**
```rust
#[component]
fn Counter() -> Element {
    let mut count = use_signal(|| 0);
    
    rsx! {
        div {
            class: "counter",
            h2 { "Count: {count}" }
            div {
                class: "counter-controls",
                button {
                    onclick: move |_| count += 1,
                    "Increment"
                }
                button {
                    onclick: move |_| count -= 1,
                    "Decrement"
                }
                button {
                    onclick: move |_| count.set(0),
                    "Reset"
                }
            }
        }
    }
}
```

**Complex State with use_memo:**
```rust
#[component]
fn TodoList(todos: Vec<Todo>) -> Element {
    let mut filter = use_signal(|| TodoFilter::All);
    
    let filtered_todos = use_memo(move || {
        match filter() {
            TodoFilter::All => todos.clone(),
            TodoFilter::Active => todos.iter().filter(|t| !t.completed).cloned().collect(),
            TodoFilter::Completed => todos.iter().filter(|t| t.completed).cloned().collect(),
        }
    });
    
    rsx! {
        div {
            class: "todo-list",
            FilterControls { filter }
            ul {
                for todo in filtered_todos() {
                    TodoItem { key: "{todo.id}", todo: todo.clone() }
                }
            }
        }
    }
}
```

### 3. Event Handling Patterns

**Mouse Events:**
```rust
#[component]
fn InteractiveButton() -> Element {
    let mut is_pressed = use_signal(|| false);
    let mut is_hovered = use_signal(|| false);
    
    rsx! {
        button {
            class: "interactive-btn",
            class: if is_pressed() { "pressed" },
            class: if is_hovered() { "hovered" },
            onmousedown: move |_| is_pressed.set(true),
            onmouseup: move |_| is_pressed.set(false),
            onmouseleave: move |_| {
                is_pressed.set(false);
                is_hovered.set(false);
            },
            onmouseenter: move |_| is_hovered.set(true),
            onclick: move |_| {
                // Handle click logic
                println!("Button clicked!");
            },
            "Click me!"
        }
    }
}
```

**Form Input Handling:**
```rust
#[component]
fn LoginForm() -> Element {
    let mut username = use_signal(|| String::new());
    let mut password = use_signal(|| String::new());
    let mut is_submitting = use_signal(|| false);
    
    let submit_form = move |_| {
        if !username().is_empty() && !password().is_empty() {
            is_submitting.set(true);
            // Perform async login logic here
            spawn(async move {
                // Simulate API call
                tokio::time::sleep(std::time::Duration::from_secs(2)).await;
                is_submitting.set(false);
            });
        }
    };
    
    rsx! {
        form {
            class: "login-form",
            onsubmit: submit_form,
            
            div {
                class: "form-group",
                label { "Username:" }
                input {
                    r#type: "text",
                    value: "{username}",
                    oninput: move |evt| username.set(evt.value()),
                    placeholder: "Enter username"
                }
            }
            
            div {
                class: "form-group",
                label { "Password:" }
                input {
                    r#type: "password",
                    value: "{password}",
                    oninput: move |evt| password.set(evt.value()),
                    placeholder: "Enter password"
                }
            }
            
            button {
                r#type: "submit",
                disabled: is_submitting(),
                if is_submitting() {
                    "Logging in..."
                } else {
                    "Login"
                }
            }
        }
    }
}
```

### 4. Advanced Component Patterns

**Higher-Order Components (Providers):**
```rust
#[derive(Clone)]
pub struct ThemeContext {
    pub theme: Signal<Theme>,
    pub toggle_theme: Callback<()>,
}

#[component]
fn ThemeProvider(children: Element) -> Element {
    let mut theme = use_signal(|| Theme::Light);
    
    let toggle_theme = use_callback(move |_| {
        theme.set(match theme() {
            Theme::Light => Theme::Dark,
            Theme::Dark => Theme::Light,
        });
    });
    
    let context = ThemeContext {
        theme,
        toggle_theme,
    };
    
    use_context_provider(|| context);
    
    rsx! {
        div {
            class: "theme-provider",
            class: match theme() {
                Theme::Light => "light-theme",
                Theme::Dark => "dark-theme",
            },
            {children}
        }
    }
}

#[component]
fn ThemedComponent() -> Element {
    let theme_ctx = use_context::<ThemeContext>();
    
    rsx! {
        div {
            class: "themed-component",
            p { "Current theme: {theme_ctx.theme():?}" }
            button {
                onclick: move |_| theme_ctx.toggle_theme.call(()),
                "Toggle Theme"
            }
        }
    }
}
```

**Render Props Pattern:**
```rust
#[component]
fn DataLoader<T: Clone + 'static>(
    url: String,
    children: Element,
    loading: Element,
    error: Element,
) -> Element {
    let mut data = use_signal(|| None::<T>);
    let mut is_loading = use_signal(|| true);
    let mut error_state = use_signal(|| None::<String>);
    
    use_effect(move || {
        let url = url.clone();
        spawn(async move {
            match fetch_data::<T>(&url).await {
                Ok(result) => {
                    data.set(Some(result));
                    is_loading.set(false);
                }
                Err(e) => {
                    error_state.set(Some(e.to_string()));
                    is_loading.set(false);
                }
            }
        });
    });
    
    rsx! {
        div {
            class: "data-loader",
            if is_loading() {
                {loading}
            } else if let Some(_error) = error_state() {
                {error}
            } else {
                {children}
            }
        }
    }
}
```

### 5. Component Lifecycle and Effects

**Cleanup Effects:**
```rust
#[component]
fn WebSocketComponent(url: String) -> Element {
    let mut messages = use_signal(|| Vec::<String>::new());
    let mut connection_status = use_signal(|| "Connecting".to_string());
    
    use_effect(move || {
        let url = url.clone();
        
        // Setup WebSocket connection
        let ws = WebSocket::new(&url).unwrap();
        
        // Handle connection open
        let on_open = {
            let connection_status = connection_status.clone();
            Closure::wrap(Box::new(move |_: web_sys::Event| {
                connection_status.set("Connected".to_string());
            }) as Box<dyn FnMut(_)>)
        };
        
        ws.set_onopen(Some(on_open.as_ref().unchecked_ref()));
        
        // Cleanup closure to prevent memory leaks
        move || {
            ws.close().ok();
            on_open.forget();
        }
    });
    
    rsx! {
        div {
            class: "websocket-component",
            p { "Status: {connection_status}" }
            ul {
                for message in messages() {
                    li { "{message}" }
                }
            }
        }
    }
}
```

### 6. Performance Optimization

**Memoized Components:**
```rust
#[component]
fn ExpensiveList(items: Vec<ListItem>) -> Element {
    // Memoize expensive computations
    let processed_items = use_memo(move || {
        items.iter()
            .filter(|item| item.visible)
            .map(|item| process_item(item))
            .collect::<Vec<_>>()
    });
    
    rsx! {
        div {
            class: "expensive-list",
            for item in processed_items() {
                MemoizedListItem {
                    key: "{item.id}",
                    item: item.clone()
                }
            }
        }
    }
}

#[component]
fn MemoizedListItem(item: ProcessedItem) -> Element {
    // This component will only re-render if item changes
    rsx! {
        div {
            class: "list-item",
            h3 { "{item.title}" }
            p { "{item.description}" }
        }
    }
}
```

### 7. Error Boundaries

```rust
#[component]
fn ErrorBoundary(children: Element) -> Element {
    let error = use_signal(|| None::<CapturedError>);
    
    if let Some(error) = error() {
        rsx! {
            div {
                class: "error-boundary",
                h2 { "Something went wrong" }
                p { "Error: {error}" }
                button {
                    onclick: move |_| error.set(None),
                    "Try Again"
                }
            }
        }
    } else {
        rsx! {
            ErrorHandler {
                on_error: move |e| error.set(Some(e)),
                {children}
            }
        }
    }
}
```

## Best Practices

### Component Design Principles

1. **Single Responsibility**: Each component should have one clear purpose
2. **Composition over Inheritance**: Build complex UIs by composing simple components
3. **Prop Validation**: Use TypeScript-like prop types for better development experience
4. **Consistent Naming**: Use PascalCase for components and camelCase for props

### Performance Guidelines

1. **Minimize State**: Keep state as local as possible
2. **Use Signals Wisely**: Prefer `use_signal` over `use_state` for reactive data
3. **Memoize Expensive Operations**: Use `use_memo` for heavy computations
4. **Avoid Inline Functions**: Extract event handlers to reduce re-renders

### RSX Best Practices

1. **Conditional Rendering**: Use `if let`, `match`, or boolean expressions appropriately
2. **List Rendering**: Always provide `key` attributes for dynamic lists
3. **Event Handling**: Use move closures for event handlers that capture state
4. **Attribute Binding**: Use string interpolation for dynamic attributes

## Common Patterns and Solutions

### Modal/Dialog Components
```rust
#[component]
fn Modal(is_open: bool, on_close: Callback<()>, children: Element) -> Element {
    if !is_open {
        return rsx! { div { style: "display: none;" } };
    }
    
    rsx! {
        div {
            class: "modal-overlay",
            onclick: move |_| on_close.call(()),
            div {
                class: "modal-content",
                onclick: move |e| e.stop_propagation(),
                button {
                    class: "modal-close",
                    onclick: move |_| on_close.call(()),
                    "×"
                }
                {children}
            }
        }
    }
}
```

### Loading States
```rust
#[component]
fn AsyncComponent() -> Element {
    let mut data = use_resource(|| async {
        fetch_data().await
    });
    
    match data() {
        Some(Ok(data)) => rsx! {
            div { "Data: {data:?}" }
        },
        Some(Err(e)) => rsx! {
            div { class: "error", "Error: {e}" }
        },
        None => rsx! {
            div { class: "loading", "Loading..." }
        }
    }
}
```

This skill provides comprehensive patterns for building robust, performant, and maintainable Dioxus components.