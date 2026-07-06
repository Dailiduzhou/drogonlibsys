use dioxus::prelude::*;

use crate::api;
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
                                {
                                    let id = book.id;
                                    let title = book.title.clone();
                                    let author = book.author.clone();
                                    rsx! {
                                        li {
                                            Link {
                                                to: Route::BookDetail { id },
                                                class: "block p-4 rounded-lg bg-slate-800 hover:bg-slate-700",
                                                div { class: "text-white font-medium", "{title}" }
                                                div { class: "text-slate-400 text-sm", "作者：{author}" }
                                            }
                                        }
                                    }
                                }
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
