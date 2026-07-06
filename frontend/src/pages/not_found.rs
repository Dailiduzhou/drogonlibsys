use dioxus::prelude::*;

use crate::routes::Route;

#[component]
pub fn NotFound(segments: Vec<String>) -> Element {
    let path = segments.join("/");
    rsx! {
        div { class: "p-16 text-center",
            h1 { class: "text-4xl font-bold text-white mb-4", "404" }
            p { class: "text-slate-300 mb-6", "未找到路径 /{path}" }
            Link {
                to: Route::Books {},
                class: "text-indigo-300 hover:underline",
                "返回图书列表"
            }
        }
    }
}
