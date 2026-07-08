use dioxus::prelude::*;

#[component]
pub fn CoverPlaceholder() -> Element {
    rsx! {
        div { class: "flex flex-col items-center justify-center text-slate-500",
            svg {
                class: "w-10 h-10 mb-1",
                xmlns: "http://www.w3.org/2000/svg",
                fill: "none",
                view_box: "0 0 24 24",
                stroke: "currentColor",
                stroke_width: "1.5",
                path {
                    stroke_linecap: "round",
                    stroke_linejoin: "round",
                    d: "M12 6.042A8.967 8.967 0 006 3.75c-1.052 0-2.062.18-3 .512v14.25A8.987 8.987 0 016 18c2.305 0 4.408.867 6 2.292m0-14.25a8.966 8.966 0 016-2.292c1.052 0 2.062.18 3 .512v14.25A8.987 8.987 0 0018 18a8.967 8.967 0 00-6 2.292m0-14.25v14.25",
                }
            }
            span { class: "text-xs", "暂无封面" }
        }
    }
}
