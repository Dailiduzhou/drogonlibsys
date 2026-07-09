use dioxus::prelude::*;
use gloo_storage::{LocalStorage, Storage};

use crate::api::{self, BookCreate};
use crate::routes::Route;

const BOOK_CREATE_FLASH_KEY: &str = "libsys.book_create_flash";

#[derive(Default, Clone)]
struct SelectedFile {
    name: String,
    content_type: Option<String>,
    bytes: Vec<u8>,
}

#[component]
pub fn BookNew() -> Element {
    let title = use_signal(String::new);
    let author = use_signal(String::new);
    let mut description = use_signal(String::new);
    let mut stock = use_signal(|| 0_i32);
    let mut error = use_signal(|| Option::<String>::None);
    let mut loading = use_signal(|| false);
    let mut selected_file = use_signal(|| Option::<SelectedFile>::None);
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
            cover_key: String::new(),
            stock: stock(),
        };
        let file_opt = selected_file();
        spawn(async move {
            match api::create_book(payload).await {
                Ok(res) => {
                    let book_id = res.id;
                    if let Some(file) = file_opt {
                        match api::upload_cover(
                            book_id,
                            &file.name,
                            file.content_type.as_deref(),
                            file.bytes,
                        )
                        .await
                        {
                            Ok(_) => {}
                            Err(e) => {
                                let _ = LocalStorage::set(
                                    BOOK_CREATE_FLASH_KEY,
                                    format!("图书已创建，请补传封面：{e}"),
                                );
                                nav.push(Route::BookEdit { id: book_id });
                                return;
                            }
                        }
                    }
                    nav.push(Route::BookDetail { id: book_id });
                }
                Err(e) => error.set(Some(e.to_string())),
            }
            loading.set(false);
        });
    };

    let mut handle_file = move |ev: Event<FormData>| {
        let files = ev.files();
        let Some(file) = files.into_iter().next() else {
            selected_file.set(None);
            return;
        };
        spawn(async move {
            let name = file.name();
            let ct = file.content_type();
            match file.read_bytes().await {
                Ok(bytes) => {
                    selected_file.set(Some(SelectedFile {
                        name,
                        content_type: ct,
                        bytes: bytes.to_vec(),
                    }));
                }
                Err(_) => {
                    selected_file.set(None);
                }
            }
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
                div { class: "mb-4",
                    label { class: "block text-sm text-slate-300 mb-1", "封面图片" }
                    input {
                        class: "block text-slate-300 text-sm",
                        r#type: "file",
                        accept: "image/*",
                        onchange: move |ev| handle_file(ev),
                    }
                    if let Some(file) = selected_file() {
                        p { class: "text-xs text-slate-400 mt-1", "已选择：{file.name}" }
                    }
                }
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
