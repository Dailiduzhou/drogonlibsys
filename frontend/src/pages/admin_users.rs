use dioxus::prelude::*;

use crate::api;
use crate::auth;
use crate::routes::Route;

#[component]
pub fn AdminUsers() -> Element {
    let auth_state = auth::use_auth();
    let mut username = use_signal(String::new);
    let mut password = use_signal(String::new);
    let mut error = use_signal(|| Option::<String>::None);
    let mut msg = use_signal(|| Option::<String>::None);
    let mut loading = use_signal(|| false);
    let mut reload_tick = use_signal(|| 0_u32);

    let users = use_resource(move || {
        let _ = reload_tick();
        async move { api::list_users(0, 100).await }
    });

    let reload = move |_| {
        reload_tick += 1;
    };

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

    if !auth_state.read().is_admin() {
        return rsx! {
            div { class: "p-6 max-w-3xl mx-auto",
                h1 { class: "text-2xl font-bold text-white mb-4", "用户管理（管理员）" }
                p { class: "text-red-400 mb-4", "仅管理员可访问此页面。" }
                Link {
                    to: Route::Books {},
                    class: "px-3 py-1.5 rounded bg-slate-700 hover:bg-slate-600 text-white text-sm",
                    "返回图书列表"
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
                    username.set(String::new());
                    password.set(String::new());
                    msg.set(Some(format!(
                        "已创建用户 `{u}`（新用户 accessExpiresAt={}，未影响当前管理员会话）",
                        pair.access_expires_at
                    )));
                    reload_tick += 1;
                }
                Err(e) => {
                    let msg = if e.code() == 409 {
                        "用户名已存在，请更换后重试。".to_string()
                    } else {
                        e.to_string()
                    };
                    error.set(Some(msg));
                }
            }
            loading.set(false);
        });
    };

    rsx! {
        div { class: "p-6",
            div { class: "mb-6",
                h1 { class: "text-2xl font-bold text-white", "用户管理（管理员）" }
                p { class: "text-xs text-slate-400 mt-1",
                    "创建用户走 POST /api/auth/register（不覆盖当前管理员令牌）。列表/删除走 GET/DELETE /api/users。"
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
                        h2 { class: "text-lg font-semibold text-white", "用户列表" }
                        div { class: "flex items-center gap-2",
                            {match &*users.read_unchecked() {
                                Some(Ok(list)) => {
                                    let n = list.len();
                                    rsx! { span { class: "text-xs text-slate-400", "共 {n} 条" } }
                                }
                                Some(Err(_)) => rsx! { span { class: "text-xs text-red-400", "加载失败" } },
                                None => rsx! { span { class: "text-xs text-slate-400", "加载中..." } },
                            }}
                            button {
                                class: "px-2 py-1 rounded bg-slate-700 hover:bg-slate-600 text-xs",
                                onclick: reload,
                                "刷新"
                            }
                        }
                    }

                    match &*users.read_unchecked() {
                        Some(Err(e)) => rsx! {
                            div { class: "px-4 py-4 flex items-center justify-between",
                                span { class: "text-red-400 text-sm", "加载失败：{e}" }
                                button {
                                    class: "px-3 py-1 rounded bg-slate-700 hover:bg-slate-600 text-xs text-white",
                                    onclick: reload,
                                    "重试"
                                }
                            }
                        },
                        None => rsx! {
                            div { class: "px-4 py-4 text-slate-400 text-sm", "加载中..." }
                        },
                        Some(Ok(list)) if list.is_empty() => rsx! {
                            div { class: "px-4 py-4 text-slate-400 text-sm", "暂无用户" }
                        },
                        Some(Ok(list)) => rsx! {
                            div { class: "overflow-x-auto",
                                table { class: "w-full text-sm text-slate-200",
                                    thead { class: "bg-slate-700 text-slate-300",
                                        tr {
                                            th { class: "px-3 py-2 text-left", "ID" }
                                            th { class: "px-3 py-2 text-left", "用户名" }
                                            th { class: "px-3 py-2 text-left", "角色" }
                                            th { class: "px-3 py-2 text-left", "未还" }
                                            th { class: "px-3 py-2 text-left", "创建时间" }
                                            th { class: "px-3 py-2", "操作" }
                                        }
                                    }
                                    tbody {
                                        for user in list.clone() {
                                            UserRow { user: user, reload_tick: reload_tick }
                                        }
                                    }
                                }
                            }
                        },
                    }
                }
            }
        }
    }
}

#[component]
fn UserRow(user: api::User, reload_tick: Signal<u32>) -> Element {
    let is_admin = user.role == "admin";
    let has_active_loans = user.active_loans > 0;
    let mut action_msg = use_signal(|| Option::<String>::None);
    let mut action_err = use_signal(|| Option::<String>::None);
    let mut busy = use_signal(|| false);
    let mut reload_tick = reload_tick;
    let id = user.id;
    let active_loans = user.active_loans;

    let do_delete = move |_| {
        action_msg.set(None);
        action_err.set(None);
        busy.set(true);
        spawn(async move {
            match api::delete_user(id).await {
                Ok(_) => {
                    action_msg.set(Some(format!("已删除用户 #{id}")));
                    reload_tick += 1;
                }
                Err(e) => {
                    let msg = if e.code() == 409 {
                        "无法删除：该用户是唯一管理员或有未归还的借阅记录。".to_string()
                    } else {
                        e.to_string()
                    };
                    action_err.set(Some(msg));
                }
            }
            busy.set(false);
        });
    };

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
            td { class: "px-3 py-2",
                if has_active_loans {
                    span {
                        class: "px-2 py-0.5 rounded text-xs bg-amber-500/20 text-amber-200",
                        title: "该用户仍有未还书, 不能删除",
                        "{active_loans} 本未还"
                    }
                } else {
                    span { class: "text-xs text-slate-500", "—" }
                }
            }
            td { class: "px-3 py-2 text-xs text-slate-400", "{user.created_at}" }
            td { class: "px-3 py-2 text-right",
                button {
                    class: "px-2 py-1 rounded bg-red-600 hover:bg-red-500 text-xs disabled:opacity-50 disabled:hover:bg-red-600",
                    disabled: busy() || has_active_loans,
                    title: if has_active_loans { "该用户仍有未还书, 请先归还再删除" } else { "" },
                    onclick: do_delete,
                    if busy() { "删除中..." } else { "删除" }
                }
                if let Some(m) = action_msg() {
                    div { class: "text-xs text-emerald-300 mt-1", "{m}" }
                }
                if let Some(m) = action_err() {
                    div { class: "text-xs text-red-300 mt-1", "{m}" }
                }
            }
        }
    }
}
