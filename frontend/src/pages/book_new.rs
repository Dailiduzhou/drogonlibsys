use dioxus::prelude::*;

use crate::api::{self, BookCreate};
use crate::routes::Route;

#[component]
pub fn BookNew() -> Element {
    let mut title = use_signal(String::new);
    let mut author = use_signal(String::new);
    let mut description = use_signal(String::new);
    let mut cover_key = use_signal(String::new);
    let mut stock = use_signal(|| 0_i32);
    let mut error = use_signal(|| Option::<String>::None);
    let mut loading = use_signal(|| false);
    let nav = use_navigator();

    let mut submit = move |_| {
        if title().is_empty() || author().is_empty() {
            error.set(Some("请填写标题与作者".to_string()));
            return;
        }
        loading.set(true);
        error.set(None);
        let payload = BookCreate {
            title: title(),
            author: author(),
            description: description(),
            cover_key: cover_key(),
            stock: stock(),
        };
        spawn(async move {
            match api::create_book(payload).await {
                Ok(res) => {
                    nav.push(Route::BookDetail { id: res.id });
                }
                Err(e) => error.set(Some(e.to_string())),
            }
            loading.set(false);
        });
    };

    rsx! {
        div { class: "p-6 max-w-lg mx-auto",
            h1 { class: "text-2xl font-bold text-white mb-6", "新增图书" }
            if let Some(m) = error() {
                div { class: "mb-4 p-2 rounded bg-red-500/20 text-red-300 text-sm", "{m}" }
            }
            form {
                onsubmit: move |ev| { ev.prevent_default(); submit(()); },
                TextField { label: "标题", value: title }
                TextField { label: "作者", value: author }
                TextField { label: "封面 Key（可选）", value: cover_key }
                div { class: "mb-4",
                    label { class: "block text-sm text-slate-300 mb-1", "库存" }
                    input {
                        class: "w-full rounded px-3 py-2 bg-slate-700 text-white",
                        r#type: "number",
                        value: "{stock}",
                        oninput: move |e| {
                            if let Ok(v) = e.value().parse::<i32>() { stock.set(v); }
                        },
                    }
                }
                div { class: "mb-4",
                    label { class: "block text-sm text-slate-300 mb-1", "描述" }
                    textarea {
                        class: "w-full rounded px-3 py-2 bg-slate-700 text-white h-32",
                        value: "{description}",
                        oninput: move |e| description.set(e.value()),
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

#[component]
fn TextField(label: String, value: Signal<String>) -> Element {
    let mut value = value;
    rsx! {
        div { class: "mb-4",
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
