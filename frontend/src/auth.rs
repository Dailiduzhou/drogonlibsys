use dioxus::prelude::*;
use gloo_storage::{LocalStorage, Storage};

use crate::api::TokenPair;

const ACCESS_KEY: &str = "libsys.access";
const REFRESH_KEY: &str = "libsys.refresh";

#[derive(Debug, Clone, Default, PartialEq)]
pub struct AuthState {
    pub access_token: Option<String>,
    pub refresh_token: Option<String>,
}

impl AuthState {
    pub fn is_authenticated(&self) -> bool {
        self.access_token
            .as_ref()
            .map(|t| !t.is_empty())
            .unwrap_or(false)
    }
}

pub fn load_access_token() -> Option<String> {
    LocalStorage::get::<String>(ACCESS_KEY).ok()
}

pub fn load_refresh_token() -> Option<String> {
    LocalStorage::get::<String>(REFRESH_KEY).ok()
}

pub fn save_tokens(pair: &TokenPair) {
    let _ = LocalStorage::set(ACCESS_KEY, &pair.access_token);
    let _ = LocalStorage::set(REFRESH_KEY, &pair.refresh_token);
}

pub fn clear_tokens() {
    LocalStorage::delete(ACCESS_KEY);
    LocalStorage::delete(REFRESH_KEY);
}

pub fn use_auth() -> Signal<AuthState> {
    use_context::<Signal<AuthState>>()
}

pub fn provide_auth() -> Signal<AuthState> {
    let state = AuthState {
        access_token: load_access_token(),
        refresh_token: load_refresh_token(),
    };
    let signal = use_signal(|| state);
    use_context_provider(|| signal);
    signal
}
