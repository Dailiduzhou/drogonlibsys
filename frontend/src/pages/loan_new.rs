use dioxus::prelude::*;

use crate::api::{self, LoanCreate};
use crate::routes::Route;

#[component]
pub fn LoanNew() -> Element {
    let mut book_id = use_signal(|| 0_i64);
    let mut user_id = use_signal(|| 0_i64);
    let mut status = use_signal(|| "borrowed".to_string());
    let mut borrowed_at = use_signal(String::new);
    let mut returned_at = use_signal(String::new);
    let mut error = use_signal(|| Option::<String>::None);
    let mut loading = use_signal(|| false);
    let nav = use_navigator();

    let mut submit = move |_| {
        if book_id() <= 0 || user_id() <= 0 || borrowed_at().is_empty() {
            error.set(Some("请填写图书 ID、用户 ID 和借出时间".into()));
            return;
        }
        loading.set(true);
        error.set(None);
        let payload = LoanCreate {
            book_id: book_id(),
            user_id: user_id(),
            status: status(),
            borrowed_at: borrowed_at(),
            returned_at: if returned_at().is_empty() { None } else { Some(returned_at()) },
        };
        spawn(async move {
            match api::create_loan(payload).await {
                Ok(res) => { nav.push(Route::LoanDetail { id: res.id }); }
                Err(e) => {
                    let msg = if e.code() == 409 {
                        "图书库存不足或不存在，请检查后再试。".to_string()
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
        div { class: "p-6 max-w-lg mx-auto",
            Link { to: Route::Loans {}, class: "text-sm text-indigo-300 hover:underline", "← 返回列表" }
            h1 { class: "text-2xl font-bold text-white mt-3 mb-6", "新建借阅记录（管理员）" }
            if let Some(m) = error() {
                div { class: "mb-4 p-2 rounded bg-red-500/20 text-red-300 text-sm", "{m}" }
            }
            form {
                onsubmit: move |ev| { ev.prevent_default(); submit(()); },
                div { class: "mb-4",
                    label { class: "block text-sm text-slate-300 mb-1", "图书 ID" }
                    input {
                        class: "w-full rounded px-3 py-2 bg-slate-700 text-white",
                        r#type: "number",
                        value: "{book_id}",
                        oninput: move |e| { if let Ok(v) = e.value().parse::<i64>() { book_id.set(v); } },
                    }
                }
                div { class: "mb-4",
                    label { class: "block text-sm text-slate-300 mb-1", "用户 ID" }
                    input {
                        class: "w-full rounded px-3 py-2 bg-slate-700 text-white",
                        r#type: "number",
                        value: "{user_id}",
                        oninput: move |e| { if let Ok(v) = e.value().parse::<i64>() { user_id.set(v); } },
                    }
                }
                div { class: "mb-4",
                    label { class: "block text-sm text-slate-300 mb-1", "状态" }
                    select {
                        class: "w-full rounded px-3 py-2 bg-slate-700 text-white",
                        value: "{status}",
                        onchange: move |e| status.set(e.value()),
                        option { value: "borrowed", "borrowed" }
                        option { value: "returned", "returned" }
                    }
                }
                div { class: "mb-4",
                    label { class: "block text-sm text-slate-300 mb-1", "借出时间 (RFC3339)" }
                    input {
                        class: "w-full rounded px-3 py-2 bg-slate-700 text-white",
                        r#type: "text",
                        placeholder: "2026-01-01T00:00:00Z",
                        value: "{borrowed_at}",
                        oninput: move |e| borrowed_at.set(e.value()),
                    }
                }
                div { class: "mb-6",
                    label { class: "block text-sm text-slate-300 mb-1", "归还时间（可选）" }
                    input {
                        class: "w-full rounded px-3 py-2 bg-slate-700 text-white",
                        r#type: "text",
                        placeholder: "2026-01-08T00:00:00Z",
                        value: "{returned_at}",
                        oninput: move |e| returned_at.set(e.value()),
                    }
                }
                button {
                    class: "w-full rounded bg-emerald-500 hover:bg-emerald-400 text-white py-2 font-medium disabled:opacity-50",
                    r#type: "submit",
                    disabled: loading(),
                    if loading() { "提交中..." } else { "创建" }
                }
            }
        }
    }
}
