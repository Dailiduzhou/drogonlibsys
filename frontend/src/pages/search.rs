use dioxus::prelude::*;

use crate::api;
use crate::components::CoverPlaceholder;
use crate::routes::Route;

#[component]
pub fn Search() -> Element {
    let mut query = use_signal(String::new);
    let mut submitted = use_signal(String::new);

    let results = use_resource(move || {
        let q = submitted();
        async move {
            if q.is_empty() {
                Ok(Vec::new())
            } else {
                api::search_books(&q, 0, 20).await
            }
        }
    });

    rsx! {
        div { class: "p-6 max-w-3xl mx-auto",
            h1 { class: "text-2xl font-bold text-white mb-4", "搜索图书" }
            form {
                onsubmit: move |ev| {
                    ev.prevent_default();
                    submitted.set(query());
                },
                class: "flex gap-2 mb-6",
                input {
                    class: "flex-1 rounded px-3 py-2 bg-slate-700 text-white",
                    r#type: "text",
                    placeholder: "输入关键词",
                    value: "{query}",
                    oninput: move |e| query.set(e.value()),
                }
                button {
                    class: "px-4 py-2 rounded bg-indigo-500 hover:bg-indigo-400 text-white",
                    r#type: "submit",
                    "搜索"
                }
            }

            if submitted().is_empty() {
                div { class: "text-slate-400", "请输入关键词后搜索" }
            } else {
                match &*results.read_unchecked() {
                    Some(Ok(list)) if list.is_empty() => rsx! {
                        div { class: "text-slate-400", "无匹配结果" }
                    },
                    Some(Ok(list)) => rsx! {
                        ul { class: "space-y-3",
                            for book in list.clone() {
                                SearchResultRow { book: book }
                            }
                        }
                    },
                    Some(Err(e)) => rsx! { div { class: "text-red-400", "搜索失败：{e}" } },
                    None => rsx! { div { class: "text-slate-400", "搜索中..." } },
                }
            }
        }
    }
}

#[component]
fn SearchResultRow(book: api::Book) -> Element {
    let id = book.id;
    let cover_src = api::cover_url(&book.cover_key);
    let has_cover = !cover_src.is_empty();
    let mut img_err = use_signal(|| false);

    rsx! {
        li {
            Link {
                to: Route::BookDetail { id },
                class: "flex gap-4 p-3 rounded-lg bg-slate-800 hover:bg-slate-700",
                div { class: "w-20 h-28 bg-slate-700 rounded overflow-hidden flex items-center justify-center shrink-0",
                    if has_cover && !img_err() {
                        img {
                            src: "{cover_src}",
                            class: "w-full h-full object-cover",
                            loading: "lazy",
                            onerror: move |_| img_err.set(true),
                        }
                    } else {
                        CoverPlaceholder {}
                    }
                }
                div { class: "flex-1 min-w-0 flex flex-col justify-center",
                    div { class: "text-white font-medium truncate", "{book.title}" }
                    div { class: "text-slate-400 text-sm", "作者：{book.author}" }
                }
            }
        }
    }
}
