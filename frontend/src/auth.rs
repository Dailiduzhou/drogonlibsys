use dioxus::prelude::*;
use gloo_storage::{LocalStorage, Storage};

use crate::api::TokenPair;

const ACCESS_KEY: &str = "libsys.access";
const REFRESH_KEY: &str = "libsys.refresh";
const ROLE_KEY: &str = "libsys.role";
const USERNAME_KEY: &str = "libsys.username";
const USERID_KEY: &str = "libsys.userId";

#[derive(Debug, Clone, Default, PartialEq)]
pub struct AuthState {
    pub access_token: Option<String>,
    pub refresh_token: Option<String>,
    pub user_id: Option<i64>,
    pub username: Option<String>,
    pub role: Option<String>,
}

impl AuthState {
    /// 仅 access token 存在且非空 -> 视为已登录 (前端 UI 门禁用,
    /// 真伪/有效性仍以后端 JwtAuthFilter 为准).
    pub fn is_authenticated(&self) -> bool {
        self.access_token
            .as_ref()
            .map(|t| !t.is_empty())
            .unwrap_or(false)
    }

    /// role == "admin". 仅为 UI 展示控制, 真正权限隔离在后端.
    pub fn is_admin(&self) -> bool {
        self.role.as_deref() == Some("admin")
    }

    pub fn from_tokens(pair: &TokenPair) -> Self {
        Self {
            access_token: Some(pair.access_token.clone()),
            refresh_token: Some(pair.refresh_token.clone()),
            user_id: if pair.user_id != 0 {
                Some(pair.user_id)
            } else {
                None
            },
            username: if pair.username.is_empty() {
                None
            } else {
                Some(pair.username.clone())
            },
            role: if pair.role.is_empty() {
                None
            } else {
                Some(pair.role.clone())
            },
        }
    }
}

pub fn load_access_token() -> Option<String> {
    LocalStorage::get::<String>(ACCESS_KEY).ok()
}

pub fn load_refresh_token() -> Option<String> {
    LocalStorage::get::<String>(REFRESH_KEY).ok()
}

fn load_role() -> Option<String> {
    LocalStorage::get::<String>(ROLE_KEY).ok()
}

fn load_username() -> Option<String> {
    LocalStorage::get::<String>(USERNAME_KEY).ok()
}

fn load_user_id() -> Option<i64> {
    LocalStorage::get::<String>(USERID_KEY)
        .ok()
        .and_then(|s| s.parse::<i64>().ok())
}

pub fn save_tokens(pair: &TokenPair) {
    let _ = LocalStorage::set(ACCESS_KEY, &pair.access_token);
    let _ = LocalStorage::set(REFRESH_KEY, &pair.refresh_token);
    let _ = LocalStorage::set(ROLE_KEY, &pair.role);
    let _ = LocalStorage::set(USERNAME_KEY, &pair.username);
    let _ = LocalStorage::set(USERID_KEY, pair.user_id.to_string());
}

/// 仅更新 token (refresh 后保留原有 user 上下文).
pub fn save_tokens_only(access: &str, refresh: &str) {
    let _ = LocalStorage::set(ACCESS_KEY, access);
    let _ = LocalStorage::set(REFRESH_KEY, refresh);
}

pub fn clear_tokens() {
    LocalStorage::delete(ACCESS_KEY);
    LocalStorage::delete(REFRESH_KEY);
    LocalStorage::delete(ROLE_KEY);
    LocalStorage::delete(USERNAME_KEY);
    LocalStorage::delete(USERID_KEY);
}

pub fn snapshot() -> AuthState {
    AuthState {
        access_token: load_access_token(),
        refresh_token: load_refresh_token(),
        user_id: load_user_id(),
        username: load_username(),
        role: load_role(),
    }
}

pub fn use_auth() -> Signal<AuthState> {
    use_context::<Signal<AuthState>>()
}
