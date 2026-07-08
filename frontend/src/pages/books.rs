use dioxus::prelude::*;

use crate::api;
use crate::auth;
use crate::components::CoverPlaceholder;
use crate::routes::Route;

#[component]
pub fn Books() -> Element {
    let auth_state = auth::use_auth();
    let offset = use_signal(|| 0_i32);
    let limit = use_signal(|| 20_i32);

    let books = use_resource(move || {
        let (off, lim) = (offset(), limit());
        async move { api::list_books(off, lim).await }
    });

    rsx! {
        div { class: "p-6",
            div { class: "flex items-center justify-between mb-6",
                h1 { class: "text-2xl font-bold text-white", "图书列表" }
                if auth_state.read().is_authenticated() {
                    Link {
                        to: Route::BookNew {},
                        class: "px-3 py-1.5 rounded bg-emerald-500 hover:bg-emerald-400 text-white text-sm",
                        "新增图书"
                    }
                }
            }

            match &*books.read_unchecked() {
                Some(Ok(list)) if list.is_empty() => rsx! {
                    div { class: "text-slate-400", "暂无图书" }
                },
                Some(Ok(list)) => rsx! {
                    div { class: "grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4",
                        for book in list.clone() {
                            BookCard { book: book }
                        }
                    }
                    Pagination { offset, limit }
                },
                Some(Err(e)) => rsx! {
                    div { class: "text-red-400", "加载失败：{e}" }
                },
                None => rsx! {
                    div { class: "text-slate-400", "加载中..." }
                },
            }
        }
    }
}

#[component]
fn BookCard(book: api::Book) -> Element {
    let id = book.id;
    let cover_src = api::cover_url(&book.cover_key);
    let has_cover = !cover_src.is_empty();
    let mut img_err = use_signal(|| false);

    rsx! {
        Link {
            to: Route::BookDetail { id },
            class: "block p-4 rounded-lg bg-slate-800 hover:bg-slate-700 transition",
            div { class: "w-full h-40 bg-slate-700 rounded mb-3 overflow-hidden flex items-center justify-center",
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
            h2 { class: "text-lg font-semibold text-white mb-1", "{book.title}" }
            p { class: "text-sm text-slate-300 mb-2", "作者：{book.author}" }
            p { class: "text-sm text-slate-400", "库存：{book.stock}" }
        }
    }
}

#[component]
fn Pagination(offset: Signal<i32>, limit: Signal<i32>) -> Element {
    let mut offset = offset;
    let limit = limit;
    rsx! {
        div { class: "flex items-center justify-center gap-2 mt-6",
            button {
                class: "px-3 py-1 rounded bg-slate-700 text-white text-sm disabled:opacity-40",
                disabled: offset() == 0,
                onclick: move |_| {
                    let step = limit();
                    let next = (offset() - step).max(0);
                    offset.set(next);
                },
                "上一页"
            }
            span { class: "text-slate-300 text-sm", "offset {offset} / limit {limit}" }
            button {
                class: "px-3 py-1 rounded bg-slate-700 text-white text-sm",
                onclick: move |_| {
                    let step = limit();
                    offset.set(offset() + step);
                },
                "下一页"
            }
        }
    }
}
