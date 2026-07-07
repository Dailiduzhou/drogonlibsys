use dioxus::prelude::*;

use crate::layout::NavLayout;
use crate::pages::{
    admin_books::AdminBooks, admin_users::AdminUsers, book_detail::BookDetail, book_edit::BookEdit,
    book_new::BookNew, books::Books, loan_detail::LoanDetail, loan_new::LoanNew, loans::Loans,
    login::Login, not_found::NotFound, search::Search,
};

#[derive(Routable, Clone, PartialEq, Debug)]
#[rustfmt::skip]
pub enum Route {
    #[layout(NavLayout)]
        #[route("/")]
        Home {},
        #[route("/login")]
        Login {},
        #[route("/books")]
        Books {},
        #[route("/books/new")]
        BookNew {},
        #[route("/books/:id")]
        BookDetail { id: i64 },
        #[route("/books/:id/edit")]
        BookEdit { id: i64 },
        #[route("/search")]
        Search {},
        #[route("/loans")]
        Loans {},
        #[route("/loans/new")]
        LoanNew {},
        #[route("/loans/:id")]
        LoanDetail { id: i64 },
        #[route("/admin/books")]
        AdminBooks {},
        #[route("/admin/users")]
        AdminUsers {},
    #[end_layout]
    #[route("/:..segments")]
    NotFound { segments: Vec<String> },
}

#[component]
pub fn Home() -> Element {
    rsx! {
        div { class: "p-8 max-w-3xl mx-auto",
            h1 { class: "text-3xl font-bold text-white mb-4", "图书管理系统" }
            p { class: "text-slate-300 mb-6",
                "基于 Drogon + PostgreSQL + Redis + MinIO 的后端服务。使用左上导航访问各功能。"
            }
            ul { class: "list-disc pl-6 text-slate-200 space-y-1",
                li { Link { to: Route::Books {}, class: "text-indigo-300 hover:underline", "浏览图书" } }
                li { Link { to: Route::Search {}, class: "text-indigo-300 hover:underline", "全文搜索" } }
                li { Link { to: Route::Loans {}, class: "text-indigo-300 hover:underline", "借阅记录" } }
                li { Link { to: Route::Login {}, class: "text-indigo-300 hover:underline", "登录" } }
                li { Link { to: Route::AdminBooks {}, class: "text-indigo-300 hover:underline", "管理：图书（管理员）" } }
                li { Link { to: Route::AdminUsers {}, class: "text-indigo-300 hover:underline", "管理：用户（管理员）" } }
            }
        }
    }
}
