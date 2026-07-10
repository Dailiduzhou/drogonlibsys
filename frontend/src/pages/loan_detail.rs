use dioxus::prelude::*;

use crate::api::{self, LoanUpdate};
use crate::auth;
use crate::routes::Route;

#[component]
pub fn LoanDetail(id: i64) -> Element {
    let auth_state = auth::use_auth();
    let loan = use_resource(move || async move { api::get_loan(id).await });
    let nav = use_navigator();

    let mut book_id = use_signal(|| 0_i64);
    let mut user_id = use_signal(|| 0_i64);
    let mut status = use_signal(|| "borrowed".to_string());
    let mut borrowed_at = use_signal(String::new);
    let mut returned_at = use_signal(String::new);
    let mut initialized = use_signal(|| false);
    let mut error = use_signal(|| Option::<String>::None);
    let mut msg = use_signal(|| Option::<String>::None);

    use_effect(move || {
        if initialized() {
            return;
        }
        if let Some(Ok(r)) = loan.read_unchecked().as_ref() {
            book_id.set(r.book_id);
            user_id.set(r.user_id);
            status.set(r.status.clone());
            borrowed_at.set(r.borrowed_at.clone());
            returned_at.set(r.returned_at.clone().unwrap_or_default());
            initialized.set(true);
        }
    });

    let mut save = move |_| {
        error.set(None);
        msg.set(None);
        let payload = LoanUpdate {
            book_id: book_id(),
            user_id: user_id(),
            status: status(),
            borrowed_at: borrowed_at(),
            returned_at: if returned_at().is_empty() {
                None
            } else {
                Some(returned_at())
            },
        };
        spawn(async move {
            match api::update_loan(id, payload).await {
                Ok(_) => msg.set(Some("更新成功".into())),
                Err(e) => {
                    let msg = if e.code() == 409 {
                        "库存不足，无法将状态改回 borrowed。".to_string()
                    } else {
                        e.to_string()
                    };
                    error.set(Some(msg));
                }
            }
        });
    };

    let mut delete = move |_| {
        error.set(None);
        msg.set(None);
        spawn(async move {
            match api::delete_loan(id).await {
                Ok(_) => {
                    nav.push(Route::Loans {});
                }
                Err(e) => error.set(Some(e.to_string())),
            }
        });
    };

    rsx! {
        div { class: "p-6 max-w-lg mx-auto",
            Link { to: Route::Loans {}, class: "text-sm text-indigo-300 hover:underline", "← 返回列表" }
            h1 { class: "text-2xl font-bold text-white mt-3 mb-6", "借阅记录 #{id}" }

            match &*loan.read_unchecked() {
                Some(Ok(_)) => rsx! {
                    if let Some(m) = error() {
                        div { class: "mb-4 p-2 rounded bg-red-500/20 text-red-300 text-sm", "{m}" }
                    }
                    if let Some(m) = msg() {
                        div { class: "mb-4 p-2 rounded bg-emerald-500/20 text-emerald-300 text-sm", "{m}" }
                    }
                    div { class: "space-y-4 rounded-lg bg-slate-800 p-6",
                        NumField { label: "图书 ID", value: book_id }
                        NumField { label: "用户 ID", value: user_id }
                        div {
                            label { class: "block text-sm text-slate-300 mb-1", "状态" }
                            select {
                                class: "w-full rounded px-3 py-2 bg-slate-700 text-white",
                                value: "{status}",
                                onchange: move |e| status.set(e.value()),
                                option { value: "borrowed", "borrowed" }
                                option { value: "returned", "returned" }
                            }
                        }
                        StrField { label: "借出时间", value: borrowed_at }
                        StrField { label: "归还时间", value: returned_at }

                        if auth_state.read().is_authenticated() {
                            div { class: "flex gap-2 pt-2",
                                button {
                                    class: "px-3 py-1.5 rounded bg-indigo-500 hover:bg-indigo-400 text-white text-sm",
                                    onclick: move |_| save(()),
                                    "保存（管理员）"
                                }
                                button {
                                    class: "px-3 py-1.5 rounded bg-red-500 hover:bg-red-400 text-white text-sm",
                                    onclick: move |_| delete(()),
                                    "删除（管理员）"
                                }
                            }
                        }
                    }
                },
                Some(Err(e)) => rsx! { div { class: "text-red-400", "加载失败：{e}" } },
                None => rsx! { div { class: "text-slate-400", "加载中..." } },
            }
        }
    }
}

#[component]
fn NumField(label: String, value: Signal<i64>) -> Element {
    let mut value = value;
    rsx! {
        div {
            label { class: "block text-sm text-slate-300 mb-1", "{label}" }
            input {
                class: "w-full rounded px-3 py-2 bg-slate-700 text-white",
                r#type: "number",
                value: "{value}",
                oninput: move |e| { if let Ok(v) = e.value().parse::<i64>() { value.set(v); } },
            }
        }
    }
}

#[component]
fn StrField(label: String, value: Signal<String>) -> Element {
    let mut value = value;
    rsx! {
        div {
            label { class: "block text-sm text-slate-300 mb-1", "{label}" }
            input {
                class: "w-full rounded px-3 py-2 bg-slate-700 text-white",
                r#type: "text",
                value: "{value}",
                oninput: move |e| value.set(e.value()),
            }
        }
    }
}
