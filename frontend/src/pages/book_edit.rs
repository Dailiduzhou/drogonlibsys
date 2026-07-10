use dioxus::prelude::*;
use gloo_storage::{LocalStorage, Storage};

use crate::api::{self, BookUpdate};
use crate::routes::Route;

const BOOK_CREATE_FLASH_KEY: &str = "libsys.book_create_flash";

#[component]
pub fn BookEdit(id: i64) -> Element {
    let mut title = use_signal(String::new);
    let mut author = use_signal(String::new);
    let mut description = use_signal(String::new);
    let mut cover_key = use_signal(String::new);
    let mut stock = use_signal(|| 0_i64);
    let mut error = use_signal(|| Option::<String>::None);
    let mut msg = use_signal(|| Option::<String>::None);
    let mut loading = use_signal(|| false);
    let mut initialized = use_signal(|| false);

    let book = use_resource(move || async move { api::get_book(id).await });

    use_effect(move || {
        if initialized() {
            return;
        }
        if let Some(Ok(b)) = book.read_unchecked().as_ref() {
            title.set(b.title.clone());
            author.set(b.author.clone());
            description.set(b.description.clone());
            cover_key.set(b.cover_key.clone());
            stock.set(b.stock);
            initialized.set(true);
        }
    });

    use_effect(move || {
        if let Ok::<String, _>(flash) = LocalStorage::get(BOOK_CREATE_FLASH_KEY) {
            msg.set(Some(flash));
            LocalStorage::delete(BOOK_CREATE_FLASH_KEY);
        }
    });

    let mut submit = move |_| {
        loading.set(true);
        error.set(None);
        msg.set(None);
        let payload = BookUpdate {
            title: title(),
            author: author(),
            description: description(),
            cover_key: cover_key(),
            stock: stock(),
        };
        spawn(async move {
            match api::update_book(id, payload).await {
                Ok(_) => msg.set(Some("更新成功".into())),
                Err(e) => error.set(Some(e.to_string())),
            }
            loading.set(false);
        });
    };

    let mut upload_cover = move |ev: Event<FormData>| {
        let files = ev.files();
        let Some(file) = files.into_iter().next() else {
            return;
        };

        msg.set(None);
        error.set(None);
        loading.set(true);
        spawn(async move {
            let name = file.name();
            let ct = file.content_type();
            match file.read_bytes().await {
                Ok(bytes) => {
                    let bytes_vec = bytes.to_vec();
                    match api::upload_cover(id, &name, ct.as_deref(), bytes_vec).await {
                        Ok(result) => {
                            cover_key.set(result.cover_key.clone());
                            msg.set(Some(format!("上传成功：{}", result.cover_url)));
                        }
                        Err(e) => error.set(Some(e.to_string())),
                    }
                }
                Err(e) => error.set(Some(format!("读取文件失败：{e:?}"))),
            }
            loading.set(false);
        });
    };

    rsx! {
        div { class: "p-6 max-w-lg mx-auto",
            Link { to: Route::BookDetail { id }, class: "text-sm text-indigo-300 hover:underline", "← 返回详情" }
            h1 { class: "text-2xl font-bold text-white mt-3 mb-6", "编辑图书 #{id}" }

            if let Some(m) = error() {
                div { class: "mb-4 p-2 rounded bg-red-500/20 text-red-300 text-sm", "{m}" }
            }
            if let Some(m) = msg() {
                div { class: "mb-4 p-2 rounded bg-emerald-500/20 text-emerald-300 text-sm", "{m}" }
            }

            form {
                onsubmit: move |ev| { ev.prevent_default(); submit(()); },
                TextField { label: "标题", value: title }
                TextField { label: "作者", value: author }
                TextField { label: "封面 Key", value: cover_key }
                div { class: "mb-4",
                    label { class: "block text-sm text-slate-300 mb-1", "库存" }
                    input {
                        class: "w-full rounded px-3 py-2 bg-slate-700 text-white",
                        r#type: "number",
                        value: "{stock}",
                        oninput: move |e| {
                            if let Ok(v) = e.value().parse::<i64>() { stock.set(v); }
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
                    class: "w-full rounded bg-indigo-500 hover:bg-indigo-400 text-white py-2 font-medium disabled:opacity-50",
                    r#type: "submit",
                    disabled: loading(),
                    if loading() { "保存中..." } else { "保存" }
                }
            }

            div { class: "mt-6 rounded-lg bg-slate-800 p-4",
                h2 { class: "text-lg font-semibold text-white mb-2", "上传封面" }
                input {
                    class: "block text-slate-300 text-sm",
                    r#type: "file",
                    accept: "image/*",
                    onchange: move |ev| upload_cover(ev),
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
