use dioxus::prelude::*;

use crate::api;
use crate::auth;
use crate::routes::Route;

#[component]
pub fn AdminBooks() -> Element {
    let auth_state = auth::use_auth();
    let offset = use_signal(|| 0_i32);
    let limit = use_signal(|| 20_i32);
    let mut reload_tick = use_signal(|| 0_u32);

    let books = use_resource(move || {
        let (off, lim) = (offset(), limit());
        let _ = reload_tick();
        async move { api::list_books(off, lim).await }
    });

    let reload = move |_| {
        reload_tick += 1;
    };

    if !auth_state.read().is_authenticated() {
        return rsx! {
            div { class: "p-6 max-w-3xl mx-auto",
                h1 { class: "text-2xl font-bold text-white mb-4", "图书管理（管理员）" }
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
                h1 { class: "text-2xl font-bold text-white mb-4", "图书管理（管理员）" }
                p { class: "text-red-400 mb-4", "仅管理员可访问此页面。" }
                Link {
                    to: Route::Books {},
                    class: "px-3 py-1.5 rounded bg-slate-700 hover:bg-slate-600 text-white text-sm",
                    "返回图书列表"
                }
            }
        };
    }

    rsx! {
        div { class: "p-6",
            div { class: "flex items-center justify-between mb-6",
                div {
                    h1 { class: "text-2xl font-bold text-white", "图书管理（管理员）" }
                    p { class: "text-xs text-slate-400 mt-1",
                        "来自 GET /api/books，CRUD 需鉴权。表格内可直接编辑/删除。"
                    }
                }
                div { class: "flex gap-2",
                    button {
                        class: "px-3 py-1.5 rounded bg-slate-700 hover:bg-slate-600 text-white text-sm",
                        onclick: reload,
                        "刷新"
                    }
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
                    div { class: "overflow-x-auto rounded-lg bg-slate-800",
                        table { class: "w-full text-sm text-slate-200",
                            thead { class: "bg-slate-700 text-slate-300",
                                tr {
                                    th { class: "px-3 py-2 text-left", "ID" }
                                    th { class: "px-3 py-2 text-left", "标题" }
                                    th { class: "px-3 py-2 text-left", "作者" }
                                    th { class: "px-3 py-2 text-left", "库存" }
                                    th { class: "px-3 py-2 text-left", "封面" }
                                    th { class: "px-3 py-2 text-left", "更新时间" }
                                    th { class: "px-3 py-2", "操作" }
                                }
                            }
                            tbody {
                                for book in list.clone() {
                                    BookAdminRow {
                                        book: book,
                                        reload_tick: reload_tick,
                                    }
                                }
                            }
                        }
                    }
                    AdminPagination { offset, limit }
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
fn BookAdminRow(book: api::Book, reload_tick: Signal<u32>) -> Element {
    let id = book.id;
    let mut action_msg = use_signal(|| Option::<String>::None);
    let mut action_err = use_signal(|| Option::<String>::None);
    let mut busy = use_signal(|| false);
    let has_cover = !book.cover_key.is_empty();
    let mut reload_tick = reload_tick;

    let do_delete = move |_| {
        action_msg.set(None);
        action_err.set(None);
        busy.set(true);
        spawn(async move {
            match api::delete_book(id).await {
                Ok(_) => {
                    action_msg.set(Some(format!("已删除图书 #{id}")));
                    reload_tick += 1;
                }
                Err(e) => {
                    let msg = if e.code() == 409 {
                        "无法删除：该图书尚有未归还的借阅记录。".to_string()
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
            td { class: "px-3 py-2 text-slate-400", "{book.id}" }
            td { class: "px-3 py-2", "{book.title}" }
            td { class: "px-3 py-2 text-slate-300", "{book.author}" }
            td { class: "px-3 py-2", "{book.stock}" }
            td { class: "px-3 py-2 text-xs text-slate-400",
                if has_cover { "{book.cover_key}" } else { "—" }
            }
            td { class: "px-3 py-2 text-xs text-slate-400", "{book.updated_at}" }
            td { class: "px-3 py-2 whitespace-nowrap",
                div { class: "flex gap-1 justify-end",
                    Link {
                        to: Route::BookDetail { id },
                        class: "px-2 py-1 rounded bg-slate-700 hover:bg-slate-600 text-xs",
                        "详情"
                    }
                    Link {
                        to: Route::BookEdit { id },
                        class: "px-2 py-1 rounded bg-indigo-600 hover:bg-indigo-500 text-xs",
                        "编辑"
                    }
                    button {
                        class: "px-2 py-1 rounded bg-red-600 hover:bg-red-500 text-xs disabled:opacity-50",
                        disabled: busy(),
                        onclick: do_delete,
                        if busy() { "删除中..." } else { "删除" }
                    }
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

#[component]
fn AdminPagination(offset: Signal<i32>, limit: Signal<i32>) -> Element {
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
