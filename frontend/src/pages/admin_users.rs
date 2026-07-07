use dioxus::prelude::*;

use crate::api;
use crate::auth;
use crate::routes::Route;

#[derive(Debug, Clone, PartialEq)]
struct MockUser {
    id: i64,
    username: String,
    role: String,
    created_at: String,
}

fn seed_users() -> Vec<MockUser> {
    vec![
        MockUser {
            id: 1,
            username: "admin".to_string(),
            role: "admin".to_string(),
            created_at: "2026-01-01T00:00:00Z".to_string(),
        },
        MockUser {
            id: 2,
            username: "alice".to_string(),
            role: "user".to_string(),
            created_at: "2026-02-01T00:00:00Z".to_string(),
        },
        MockUser {
            id: 3,
            username: "bob".to_string(),
            role: "user".to_string(),
            created_at: "2026-03-01T00:00:00Z".to_string(),
        },
    ]
}

#[component]
pub fn AdminUsers() -> Element {
    let auth_state = auth::use_auth();
    let mut username = use_signal(String::new);
    let mut password = use_signal(String::new);
    let mut error = use_signal(|| Option::<String>::None);
    let mut msg = use_signal(|| Option::<String>::None);
    let mut loading = use_signal(|| false);
    let mut users = use_signal(seed_users);

    if !auth_state.read().is_authenticated() {
        return rsx! {
            div { class: "p-6 max-w-3xl mx-auto",
                h1 { class: "text-2xl font-bold text-white mb-4", "用户管理（管理员）" }
                p { class: "text-slate-300 mb-4", "该页面需要登录后访问。" }
                Link {
                    to: Route::Login {},
                    class: "px-3 py-1.5 rounded bg-indigo-500 hover:bg-indigo-400 text-white text-sm",
                    "去登录"
                }
            }
        };
    }

    let mut submit = move |_| {
        let u = username();
        let p = password();
        if u.is_empty() || p.is_empty() {
            error.set(Some("请填写用户名与密码".to_string()));
            return;
        }
        loading.set(true);
        error.set(None);
        msg.set(None);
        spawn(async move {
            match api::admin_create_user(&u, &p).await {
                Ok(pair) => {
                    let next_id = users().iter().map(|x| x.id).max().unwrap_or(0) + 1;
                    users.write().insert(
                        0,
                        MockUser {
                            id: next_id,
                            username: u.clone(),
                            role: "user".to_string(),
                            created_at: "2026-07-07T00:00:00Z".to_string(),
                        },
                    );
                    username.set(String::new());
                    password.set(String::new());
                    msg.set(Some(format!(
                        "已创建用户 `{u}`（新用户 accessExpiresAt={}，未影响当前管理员会话）",
                        pair.access_expires_at
                    )));
                }
                Err(e) => error.set(Some(e.to_string())),
            }
            loading.set(false);
        });
    };

    rsx! {
        div { class: "p-6",
            div { class: "mb-6",
                h1 { class: "text-2xl font-bold text-white", "用户管理（管理员）" }
                p { class: "text-xs text-slate-400 mt-1",
                    "创建用户走 POST /api/auth/register（不覆盖当前管理员令牌）。"
                }
                p { class: "text-xs text-amber-400 mt-1",
                    "注意：openapi 暂未提供用户列表 / 删除接口，下表为占位 Mock 数据，待后端补齐后接 GET /api/users 等。"
                }
            }

            div { class: "grid grid-cols-1 lg:grid-cols-2 gap-6",
                div { class: "rounded-lg bg-slate-800 p-5",
                    h2 { class: "text-lg font-semibold text-white mb-3", "创建用户" }
                    if let Some(m) = error() {
                        div { class: "mb-3 p-2 rounded bg-red-500/20 text-red-300 text-sm", "{m}" }
                    }
                    if let Some(m) = msg() {
                        div { class: "mb-3 p-2 rounded bg-emerald-500/20 text-emerald-300 text-sm", "{m}" }
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
                        div { class: "mb-4",
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
                            if loading() { "创建中..." } else { "创建" }
                        }
                    }
                }

                div { class: "rounded-lg bg-slate-800 overflow-hidden",
                    div { class: "flex items-center justify-between px-4 py-3 border-b border-slate-700",
                        h2 { class: "text-lg font-semibold text-white", "用户列表（Mock）" }
                        span { class: "text-xs text-slate-400", "共 {users().len()} 条" }
                    }
                    div { class: "overflow-x-auto",
                        table { class: "w-full text-sm text-slate-200",
                            thead { class: "bg-slate-700 text-slate-300",
                                tr {
                                    th { class: "px-3 py-2 text-left", "ID" }
                                    th { class: "px-3 py-2 text-left", "用户名" }
                                    th { class: "px-3 py-2 text-left", "角色" }
                                    th { class: "px-3 py-2 text-left", "创建时间" }
                                    th { class: "px-3 py-2", "操作" }
                                }
                            }
                            tbody {
                                for user in users().clone() {
                                    MockUserRow { user: user }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

#[component]
fn MockUserRow(user: MockUser) -> Element {
    let is_admin = user.role == "admin";
    rsx! {
        tr { class: "border-t border-slate-700",
            td { class: "px-3 py-2 text-slate-400", "{user.id}" }
            td { class: "px-3 py-2", "{user.username}" }
            td { class: "px-3 py-2",
                span {
                    class: if is_admin { "px-2 py-0.5 rounded text-xs bg-indigo-500/30 text-indigo-200" }
                        else { "px-2 py-0.5 rounded text-xs bg-slate-600 text-slate-200" },
                    "{user.role}"
                }
            }
            td { class: "px-3 py-2 text-xs text-slate-400", "{user.created_at}" }
            td { class: "px-3 py-2 text-right",
                span { class: "text-xs text-slate-500", "待接口" }
            }
        }
    }
}