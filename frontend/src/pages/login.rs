use dioxus::prelude::*;

use crate::api;
use crate::auth::{self, AuthState};
use crate::routes::Route;

#[component]
pub fn Login() -> Element {
    let mut username = use_signal(String::new);
    let mut password = use_signal(String::new);
    let mut error = use_signal(|| Option::<String>::None);
    let mut loading = use_signal(|| false);
    let mut auth_state = auth::use_auth();
    let nav = use_navigator();

    let mut submit = move |_| {
        let u = username();
        let p = password();
        if u.is_empty() || p.is_empty() {
            error.set(Some("请输入用户名与密码".to_string()));
            return;
        }
        loading.set(true);
        error.set(None);
        spawn(async move {
            match api::login(&u, &p).await {
                Ok(pair) => {
                    auth_state.set(AuthState {
                        access_token: Some(pair.access_token),
                        refresh_token: Some(pair.refresh_token),
                    });
                    nav.push(Route::Books {});
                }
                Err(e) => error.set(Some(e.to_string())),
            }
            loading.set(false);
        });
    };

    rsx! {
        div { class: "mx-auto max-w-sm p-8 mt-16 rounded-xl bg-slate-800 shadow-lg",
            h1 { class: "text-2xl font-bold mb-6 text-white", "登录" }
            if let Some(msg) = error() {
                div { class: "mb-4 p-2 rounded bg-red-500/20 text-red-300 text-sm", "{msg}" }
            }
            form {
                onsubmit: move |ev| { ev.prevent_default(); submit(()); },
                div { class: "mb-4",
                    label { class: "block text-sm text-slate-300 mb-1", "用户名" }
                    input {
                        class: "w-full rounded px-3 py-2 bg-slate-700 text-white",
                        r#type: "text",
                        value: "{username}",
                        oninput: move |e| username.set(e.value()),
                    }
                }
                div { class: "mb-6",
                    label { class: "block text-sm text-slate-300 mb-1", "密码" }
                    input {
                        class: "w-full rounded px-3 py-2 bg-slate-700 text-white",
                        r#type: "password",
                        value: "{password}",
                        oninput: move |e| password.set(e.value()),
                    }
                }
                button {
                    class: "w-full rounded bg-indigo-500 hover:bg-indigo-400 text-white py-2 font-medium disabled:opacity-50",
                    r#type: "submit",
                    disabled: loading(),
                    if loading() { "登录中..." } else { "登录" }
                }
            }
        }
    }
}
