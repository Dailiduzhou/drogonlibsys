---
name: dioxus-state-management
description: Manage application state in Dioxus using signals, stores, context, and global state patterns. Use when implementing reactive state, sharing data between components, or building complex state architectures.
---

# Dioxus State Management

This skill helps you implement effective state management in Dioxus applications using signals, stores, context providers, and global state patterns.

## When to use this skill

- Managing local component state with reactive updates
- Sharing state between components and component hierarchies
- Implementing global application state management
- Building complex state architectures with stores and context
- Optimizing state updates and preventing unnecessary re-renders
- Handling async state and data fetching patterns

## Core State Management Concepts

### 1. Local State with Signals

**Basic Signal Usage:**
```rust
#![allow(non_snake_case)]
use dioxus::prelude::*;

#[component]
fn Counter() -> Element {
    let mut count = use_signal(|| 0);
    
    rsx! {
        div {
            class: "counter",
            h2 { "Count: {count}" }
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
```

**Complex State Objects:**
```rust
#[derive(Clone, Debug, PartialEq)]
struct UserProfile {
    name: String,
    email: String,
    avatar_url: Option<String>,
    preferences: UserPreferences,
}

#[derive(Clone, Debug, PartialEq)]
struct UserPreferences {
    theme: Theme,
    notifications: bool,
    language: String,
}

#[component]
fn UserProfileEditor() -> Element {
    let mut profile = use_signal(|| UserProfile {
        name: String::new(),
        email: String::new(),
        avatar_url: None,
        preferences: UserPreferences {
            theme: Theme::Light,
            notifications: true,
            language: "en".to_string(),
        },
    });
    
    rsx! {
        div {
            class: "profile-editor",
            h2 { "Edit Profile" }
            
            input {
                r#type: "text",
                placeholder: "Name",
                value: "{profile().name}",
                oninput: move |evt| {
                    profile.write().name = evt.value();
                }
            }
            
            input {
                r#type: "email",
                placeholder: "Email",
                value: "{profile().email}",
                oninput: move |evt| {
                    profile.write().email = evt.value();
                }
            }
            
            div {
                class: "preferences",
                h3 { "Preferences" }
                
                label {
                    input {
                        r#type: "checkbox",
                        checked: profile().preferences.notifications,
                        onchange: move |evt| {
                            profile.write().preferences.notifications = evt.checked();
                        }
                    }
                    "Enable notifications"
                }
                
                select {
                    value: "{profile().preferences.theme:?}",
                    onchange: move |evt| {
                        profile.write().preferences.theme = match evt.value().as_str() {
                            "Dark" => Theme::Dark,
                            _ => Theme::Light,
                        };
                    },
                    option { value: "Light", "Light Theme" }
                    option { value: "Dark", "Dark Theme" }
                }
            }
            
            button {
                onclick: move |_| {
                    save_profile(profile());
                },
                "Save Profile"
            }
        }
    }
}
```

### 2. Derived State with use_memo

**Computed Values:**
```rust
#[component]
fn ShoppingCart() -> Element {
    let mut items = use_signal(|| Vec::<CartItem>::new());
    
    // Derived state - automatically updates when items change
    let total_items = use_memo(move || {
        items().iter().map(|item| item.quantity).sum::<u32>()
    });
    
    let total_price = use_memo(move || {
        items().iter()
            .map(|item| item.price * item.quantity as f64)
            .sum::<f64>()
    });
    
    let has_items = use_memo(move || !items().is_empty());
    
    rsx! {
        div {
            class: "shopping-cart",
            h2 { "Shopping Cart" }
            
            if has_items() {
                div {
                    class: "cart-summary",
                    p { "Items: {total_items}" }
                    p { "Total: ${total_price:.2}" }
                }
                
                div {
                    class: "cart-items",
                    for item in items() {
                        CartItemComponent {
                            key: "{item.id}",
                            item: item.clone(),
                            on_quantity_change: move |item_id, new_quantity| {
                                items.write().iter_mut()
                                    .find(|i| i.id == item_id)
                                    .map(|i| i.quantity = new_quantity);
                            },
                            on_remove: move |item_id| {
                                items.write().retain(|i| i.id != item_id);
                            }
                        }
                    }
                }
                
                button {
                    class: "checkout-btn",
                    onclick: move |_| checkout(items()),
                    "Checkout (${total_price:.2})"
                }
            } else {
                div {
                    class: "empty-cart",
                    p { "Your cart is empty" }
                }
            }
        }
    }
}
```

### 3. Context-Based State Sharing

**Authentication Context:**
```rust
#[derive(Clone)]
pub struct AuthContext {
    pub user: Signal<Option<User>>,
    pub is_loading: Signal<bool>,
    pub error: Signal<Option<String>>,
    pub login: Callback<(String, String)>,
    pub logout: Callback<()>,
    pub register: Callback<UserRegistration>,
}

#[component]
pub fn AuthProvider(children: Element) -> Element {
    let mut user = use_signal(|| None::<User>);
    let mut is_loading = use_signal(|| false);
    let mut error = use_signal(|| None::<String>);
    
    let login = use_callback(move |(email, password): (String, String)| {
        let user = user.clone();
        let is_loading = is_loading.clone();
        let error = error.clone();
        
        spawn(async move {
            is_loading.set(true);
            error.set(None);
            
            match authenticate(&email, &password).await {
                Ok(user_data) => {
                    user.set(Some(user_data));
                    store_session_token().await;
                }
                Err(e) => {
                    error.set(Some(e.to_string()));
                }
            }
            
            is_loading.set(false);
        });
    });
    
    let logout = use_callback(move |_| {
        let user = user.clone();
        spawn(async move {
            clear_session_token().await;
            user.set(None);
        });
    });
    
    let register = use_callback(move |registration: UserRegistration| {
        let user = user.clone();
        let is_loading = is_loading.clone();
        let error = error.clone();
        
        spawn(async move {
            is_loading.set(true);
            error.set(None);
            
            match create_user(&registration).await {
                Ok(user_data) => {
                    user.set(Some(user_data));
                }
                Err(e) => {
                    error.set(Some(e.to_string()));
                }
            }
            
            is_loading.set(false);
        });
    });
    
    let context = AuthContext {
        user,
        is_loading,
        error,
        login,
        logout,
        register,
    };
    
    use_context_provider(|| context);
    
    rsx! { {children} }
}

#[component]
fn LoginForm() -> Element {
    let auth = use_context::<AuthContext>();
    let mut email = use_signal(|| String::new());
    let mut password = use_signal(|| String::new());
    
    let handle_submit = move |_| {
        auth.login.call((email(), password()));
    };
    
    rsx! {
        form {
            class: "login-form",
            onsubmit: handle_submit,
            
            if let Some(error) = auth.error() {
                div {
                    class: "error-message",
                    "{error}"
                }
            }
            
            input {
                r#type: "email",
                placeholder: "Email",
                value: "{email}",
                oninput: move |evt| email.set(evt.value()),
                disabled: auth.is_loading()
            }
            
            input {
                r#type: "password",
                placeholder: "Password",
                value: "{password}",
                oninput: move |evt| password.set(evt.value()),
                disabled: auth.is_loading()
            }
            
            button {
                r#type: "submit",
                disabled: auth.is_loading(),
                if auth.is_loading() {
                    "Logging in..."
                } else {
                    "Login"
                }
            }
        }
    }
}

#[component]
fn UserProfile() -> Element {
    let auth = use_context::<AuthContext>();
    
    match auth.user() {
        Some(user) => rsx! {
            div {
                class: "user-profile",
                h2 { "Welcome, {user.name}!" }
                p { "Email: {user.email}" }
                button {
                    onclick: move |_| auth.logout.call(()),
                    "Logout"
                }
            }
        },
        None => rsx! {
            div {
                class: "not-authenticated",
                p { "Please log in to view your profile." }
            }
        }
    }
}
```

### 4. Global State with Stores

**Application Store:**
```rust
use dioxus_stores::*;

#[derive(Default, Store)]
struct AppStore {
    theme: Theme,
    user_preferences: UserPreferences,
    notifications: Vec<Notification>,
    ui_state: UiState,
}

#[derive(Default)]
struct UiState {
    sidebar_open: bool,
    modal_stack: Vec<ModalType>,
    loading_states: HashMap<String, bool>,
}

#[derive(Clone)]
enum StoreAction {
    SetTheme(Theme),
    ToggleSidebar,
    AddNotification(Notification),
    RemoveNotification(String),
    SetLoading(String, bool),
    OpenModal(ModalType),
    CloseModal,
}

impl AppStore {
    fn reduce(&mut self, action: StoreAction) {
        match action {
            StoreAction::SetTheme(theme) => {
                self.theme = theme;
            }
            StoreAction::ToggleSidebar => {
                self.ui_state.sidebar_open = !self.ui_state.sidebar_open;
            }
            StoreAction::AddNotification(notification) => {
                self.notifications.push(notification);
            }
            StoreAction::RemoveNotification(id) => {
                self.notifications.retain(|n| n.id != id);
            }
            StoreAction::SetLoading(key, loading) => {
                if loading {
                    self.ui_state.loading_states.insert(key, true);
                } else {
                    self.ui_state.loading_states.remove(&key);
                }
            }
            StoreAction::OpenModal(modal) => {
                self.ui_state.modal_stack.push(modal);
            }
            StoreAction::CloseModal => {
                self.ui_state.modal_stack.pop();
            }
        }
    }
}

#[component]
fn App() -> Element {
    let mut store = use_store(AppStore::default);
    
    rsx! {
        div {
            class: "app",
            class: match store.theme {
                Theme::Light => "light-theme",
                Theme::Dark => "dark-theme",
            },
            
            Header {
                store: store.clone(),
            }
            
            div {
                class: "main-content",
                class: if store.ui_state.sidebar_open { "sidebar-open" } else { "sidebar-closed" },
                
                Sidebar {
                    store: store.clone(),
                    open: store.ui_state.sidebar_open,
                }
                
                MainContent {
                    store: store.clone(),
                }
            }
            
            NotificationCenter {
                notifications: store.notifications.clone(),
                on_dismiss: move |id| {
                    store.write().reduce(StoreAction::RemoveNotification(id));
                }
            }
            
            ModalStack {
                modals: store.ui_state.modal_stack.clone(),
                on_close: move |_| {
                    store.write().reduce(StoreAction::CloseModal);
                }
            }
        }
    }
}
```

### 5. Async State Management

**Data Fetching with Resources:**
```rust
#[component]
fn UserList() -> Element {
    let mut users_resource = use_resource(|| async {
        fetch_users().await
    });
    
    let mut search_query = use_signal(|| String::new());
    let mut filter_role = use_signal(|| None::<UserRole>);
    
    // Refetch when filters change
    use_effect(move || {
        let query = search_query();
        let role = filter_role();
        
        users_resource.restart();
    });
    
    rsx! {
        div {
            class: "user-list",
            
            div {
                class: "filters",
                input {
                    r#type: "text",
                    placeholder: "Search users...",
                    value: "{search_query}",
                    oninput: move |evt| search_query.set(evt.value())
                }
                
                select {
                    onchange: move |evt| {
                        filter_role.set(match evt.value().as_str() {
                            "admin" => Some(UserRole::Admin),
                            "user" => Some(UserRole::User),
                            _ => None,
                        });
                    },
                    option { value: "", "All Roles" }
                    option { value: "admin", "Admin" }
                    option { value: "user", "User" }
                }
                
                button {
                    onclick: move |_| users_resource.restart(),
                    "Refresh"
                }
            }
            
            match users_resource() {
                Some(Ok(users)) => rsx! {
                    div {
                        class: "users-grid",
                        for user in users {
                            UserCard {
                                key: "{user.id}",
                                user: user.clone()
                            }
                        }
                    }
                },
                Some(Err(e)) => rsx! {
                    div {
                        class: "error-state",
                        h3 { "Failed to load users" }
                        p { "{e}" }
                        button {
                            onclick: move |_| users_resource.restart(),
                            "Try Again"
                        }
                    }
                },
                None => rsx! {
                    div {
                        class: "loading-state",
                        div { class: "spinner" }
                        p { "Loading users..." }
                    }
                }
            }
        }
    }
}
```

**Optimistic Updates:**
```rust
#[component]
fn TodoItem(todo: Todo, on_update: Callback<Todo>) -> Element {
    let mut is_updating = use_signal(|| false);
    let mut optimistic_todo = use_signal(|| todo.clone());
    
    let toggle_completed = move |_| {
        // Optimistic update
        optimistic_todo.write().completed = !optimistic_todo().completed;
        is_updating.set(true);
        
        let todo_id = todo.id;
        let new_completed = optimistic_todo().completed;
        
        spawn(async move {
            match update_todo_completed(todo_id, new_completed).await {
                Ok(updated_todo) => {
                    optimistic_todo.set(updated_todo.clone());
                    on_update.call(updated_todo);
                }
                Err(_) => {
                    // Revert optimistic update on failure
                    optimistic_todo.write().completed = !new_completed;
                }
            }
            is_updating.set(false);
        });
    };
    
    rsx! {
        div {
            class: "todo-item",
            class: if optimistic_todo().completed { "completed" } else { "pending" },
            class: if is_updating() { "updating" } else { "" },
            
            input {
                r#type: "checkbox",
                checked: optimistic_todo().completed,
                onchange: toggle_completed,
                disabled: is_updating()
            }
            
            span {
                class: "todo-text",
                "{optimistic_todo().text}"
            }
            
            if is_updating() {
                span { class: "updating-indicator", "⟳" }
            }
        }
    }
}
```

## Advanced Patterns

### 1. State Persistence

```rust
#[component]
fn PersistentSettings() -> Element {
    let mut settings = use_signal(|| {
        // Load from localStorage on init
        load_settings_from_storage().unwrap_or_default()
    });
    
    // Persist to storage whenever settings change
    use_effect(move || {
        let current_settings = settings();
        spawn(async move {
            save_settings_to_storage(&current_settings).await.ok();
        });
    });
    
    rsx! {
        div {
            class: "settings",
            SettingsForm {
                settings: settings(),
                on_change: move |new_settings| settings.set(new_settings)
            }
        }
    }
}
```

### 2. State Synchronization

```rust
#[component]
fn SyncedComponent() -> Element {
    let mut local_state = use_signal(|| AppState::default());
    let mut sync_status = use_signal(|| SyncStatus::Synced);
    
    // Sync with server periodically
    use_effect(|| {
        let interval = use_interval(Duration::from_secs(30), move || {
            sync_status.set(SyncStatus::Syncing);
            
            spawn(async move {
                match sync_with_server(&local_state()).await {
                    Ok(server_state) => {
                        local_state.set(server_state);
                        sync_status.set(SyncStatus::Synced);
                    }
                    Err(_) => {
                        sync_status.set(SyncStatus::Error);
                    }
                }
            });
        });
        
        move || interval.cancel()
    });
    
    rsx! {
        div {
            class: "synced-component",
            
            div {
                class: "sync-indicator",
                match sync_status() {
                    SyncStatus::Synced => "✓ Synced",
                    SyncStatus::Syncing => "⟳ Syncing...",
                    SyncStatus::Error => "⚠ Sync Error",
                }
            }
            
            // Component content here
        }
    }
}
```

## Best Practices

### 1. State Organization
- Keep state as local as possible
- Use context for truly global state
- Prefer composition over deep prop drilling
- Separate UI state from business logic

### 2. Performance Optimization
- Use `use_memo` for expensive computations
- Avoid unnecessary signal writes
- Structure state to minimize re-renders
- Use keys for efficient list updates

### 3. Error Handling
- Handle async state errors gracefully
- Provide fallback states for failed operations
- Implement retry mechanisms where appropriate
- Show meaningful error messages to users

### 4. Testing
- Mock state providers in tests
- Test state transitions independently
- Use deterministic state for reliable tests
- Test error conditions and edge cases

This skill provides comprehensive patterns for managing state effectively in Dioxus applications, from simple local state to complex global state architectures.