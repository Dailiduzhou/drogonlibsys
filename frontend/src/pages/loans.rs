use dioxus::prelude::*;

use crate::api;
use crate::routes::Route;

#[component]
pub fn Loans() -> Element {
    let offset = use_signal(|| 0_i32);
    let limit = use_signal(|| 20_i32);
    let loans = use_resource(move || {
        let (off, lim) = (offset(), limit());
        async move { api::list_loans(off, lim).await }
    });

    rsx! {
        div { class: "p-6",
            div { class: "flex items-center justify-between mb-6",
                h1 { class: "text-2xl font-bold text-white", "借阅记录" }
                Link {
                    to: Route::LoanNew {},
                    class: "px-3 py-1.5 rounded bg-emerald-500 hover:bg-emerald-400 text-white text-sm",
                    "新建记录（管理员）"
                }
            }
            match &*loans.read_unchecked() {
                Some(Ok(list)) if list.is_empty() => rsx! {
                    div { class: "text-slate-400", "暂无记录" }
                },
                Some(Ok(list)) => rsx! {
                    div { class: "overflow-x-auto rounded-lg bg-slate-800",
                        table { class: "w-full text-sm text-slate-200",
                            thead { class: "bg-slate-700 text-slate-300",
                                tr {
                                    th { class: "px-3 py-2 text-left", "ID" }
                                    th { class: "px-3 py-2 text-left", "图书" }
                                    th { class: "px-3 py-2 text-left", "用户" }
                                    th { class: "px-3 py-2 text-left", "状态" }
                                    th { class: "px-3 py-2 text-left", "借出时间" }
                                    th { class: "px-3 py-2 text-left", "归还时间" }
                                    th { class: "px-3 py-2", "" }
                                }
                            }
                            tbody {
                                for loan in list.clone() {
                                    LoanRow { loan: loan }
                                }
                            }
                        }
                    }
                    Pagination { offset, limit }
                },
                Some(Err(e)) => rsx! { div { class: "text-red-400", "加载失败：{e}" } },
                None => rsx! { div { class: "text-slate-400", "加载中..." } },
            }
        }
    }
}

#[component]
fn LoanRow(loan: api::LoanRecord) -> Element {
    let id = loan.id;
    let returned = loan.returned_at.clone().unwrap_or_default();
    rsx! {
        tr { class: "border-t border-slate-700",
            td { class: "px-3 py-2", "{loan.id}" }
            td { class: "px-3 py-2", "{loan.book_id}" }
            td { class: "px-3 py-2", "{loan.user_id}" }
            td { class: "px-3 py-2", "{loan.status}" }
            td { class: "px-3 py-2", "{loan.borrowed_at}" }
            td { class: "px-3 py-2", "{returned}" }
            td { class: "px-3 py-2",
                Link {
                    to: Route::LoanDetail { id },
                    class: "text-indigo-300 hover:underline",
                    "查看"
                }
            }
        }
    }
}

#[component]
fn Pagination(offset: Signal<i32>, limit: Signal<i32>) -> Element {
    let mut offset = offset;
    let limit = limit;
    rsx! {
        div { class: "flex items-center justify-center gap-2 mt-4",
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
