use dioxus::prelude::*;

use crate::api;
use crate::auth;
use crate::components::CoverPlaceholder;
use crate::routes::Route;

#[component]
pub fn BookDetail(id: i64) -> Element {
    let auth_state = auth::use_auth();
    let mut action_msg = use_signal(|| Option::<String>::None);
    let mut action_err = use_signal(|| Option::<String>::None);
    let mut reload_tick = use_signal(|| 0_u32);
    let mut img_err = use_signal(|| false);

    let book = use_resource(move || {
        let _ = reload_tick();
        async move { api::get_book(id).await }
    });
    let nav = use_navigator();

    rsx! {
        div { class: "p-6 max-w-3xl mx-auto",
            Link { to: Route::Books {}, class: "text-sm text-indigo-300 hover:underline", "← 返回列表" }
            match &*book.read_unchecked() {
                Some(Ok(b)) => rsx! {
                    div { class: "mt-4 rounded-lg bg-slate-800 p-6",
                        {
                            let cover_src = api::book_cover_url(b);
                            let has_cover = !cover_src.is_empty();
                            rsx! {
                                div { class: "w-full h-64 bg-slate-700 rounded mb-4 overflow-hidden flex items-center justify-center",
                                    if has_cover && !img_err() {
                                        img {
                                            src: "{cover_src}",
                                            class: "w-full h-full object-contain",
                                            loading: "lazy",
                                            onerror: move |_| img_err.set(true),
                                        }
                                    } else {
                                        CoverPlaceholder {}
                                    }
                                }
                            }
                        }
                        h1 { class: "text-2xl font-bold text-white mb-2", "{b.title}" }
                        p { class: "text-slate-300 mb-1", "作者：{b.author}" }
                        p { class: "text-slate-400 mb-1", "库存：{b.stock}" }
                        if !b.cover_key.is_empty() {
                            p { class: "text-slate-500 text-sm mb-1", "封面 Key：{b.cover_key}" }
                        }
                        if !b.description.is_empty() {
                            div { class: "mt-4 text-slate-200 whitespace-pre-wrap", "{b.description}" }
                        }
                        p { class: "text-xs text-slate-500 mt-4",
                            "创建：{b.created_at}　更新：{b.updated_at}"
                        }

                        if auth_state.read().is_authenticated() {
                            div { class: "mt-6 flex flex-wrap gap-2",
                                button {
                                    class: "px-3 py-1.5 rounded bg-emerald-500 hover:bg-emerald-400 text-white text-sm",
                                    onclick: move |_| {
                                        action_msg.set(None);
                                        action_err.set(None);
                                        spawn(async move {
                                            match api::borrow_book(id).await {
                                                Ok(r) => action_msg.set(Some(format!("借出成功，借阅 id={}", r.id))),
                                                Err(e) => action_err.set(Some(e.to_string())),
                                            }
                                            reload_tick += 1;
                                        });
                                    },
                                    "借阅"
                                }
                                button {
                                    class: "px-3 py-1.5 rounded bg-amber-500 hover:bg-amber-400 text-white text-sm",
                                    onclick: move |_| {
                                        action_msg.set(None);
                                        action_err.set(None);
                                        spawn(async move {
                                            match api::return_book(id).await {
                                                Ok(r) => action_msg.set(Some(format!("归还成功，借阅 id={}", r.id))),
                                                Err(e) => action_err.set(Some(e.to_string())),
                                            }
                                            reload_tick += 1;
                                        });
                                    },
                                    "归还"
                                }
                                Link {
                                    to: Route::BookEdit { id },
                                    class: "px-3 py-1.5 rounded bg-indigo-500 hover:bg-indigo-400 text-white text-sm",
                                    "编辑"
                                }
                                button {
                                    class: "px-3 py-1.5 rounded bg-red-500 hover:bg-red-400 text-white text-sm",
                                    onclick: move |_| {
                                        action_msg.set(None);
                                        action_err.set(None);
                                        spawn(async move {
                                            match api::delete_book(id).await {
                                                Ok(_) => { nav.push(Route::Books {}); }
                                                Err(e) => action_err.set(Some(e.to_string())),
                                            }
                                        });
                                    },
                                    "删除"
                                }
                            }
                        }
                        if let Some(m) = action_msg() {
                            div { class: "mt-3 text-sm text-emerald-300", "{m}" }
                        }
                        if let Some(m) = action_err() {
                            div { class: "mt-3 text-sm text-red-300", "{m}" }
                        }
                    }
                },
                Some(Err(e)) => rsx! { div { class: "mt-4 text-red-400", "加载失败：{e}" } },
                None => rsx! { div { class: "mt-4 text-slate-400", "加载中..." } },
            }
        }
    }
}
