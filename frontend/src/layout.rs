use dioxus::prelude::*;

use crate::api;
use crate::auth::{self, AuthState};
use crate::routes::Route;

#[component]
pub fn NavLayout() -> Element {
    let mut auth_state = auth::use_auth();
    let nav = use_navigator();
    let authenticated = auth_state.read().is_authenticated();

    let logout = move |_| {
        spawn(async move {
            let _ = api::logout().await;
            auth_state.set(AuthState::default());
            nav.push(Route::Login {});
        });
    };

    rsx! {
        div { class: "min-h-screen bg-slate-900 text-white",
            nav { class: "border-b border-slate-700 bg-slate-800",
                div { class: "max-w-6xl mx-auto flex items-center px-4 py-3 gap-4",
                    Link {
                        to: Route::Home {},
                        class: "text-lg font-semibold text-white mr-4",
                        "LibSys"
                    }
                    NavItem { to: Route::Books {}, label: "图书" }
                    NavItem { to: Route::Search {}, label: "搜索" }
                    NavItem { to: Route::Loans {}, label: "借阅" }
                    if authenticated {
                        NavItem { to: Route::AdminBooks {}, label: "图书管理" }
                        NavItem { to: Route::AdminUsers {}, label: "用户管理" }
                    }
                    div { class: "ml-auto flex items-center gap-3",
                        if authenticated {
                            button {
                                class: "px-3 py-1 rounded bg-slate-700 hover:bg-slate-600 text-sm",
                                onclick: move |_| logout(()),
                                "登出"
                            }
                        } else {
                            NavItem { to: Route::Login {}, label: "登录" }
                        }
                    }
                }
            }
            main { class: "max-w-6xl mx-auto",
                Outlet::<Route> {}
            }
        }
    }
}

#[component]
fn NavItem(to: Route, label: String) -> Element {
    rsx! {
        Link {
            to: to.clone(),
            class: "text-sm text-slate-200 hover:text-white",
            "{label}"
        }
    }
}
