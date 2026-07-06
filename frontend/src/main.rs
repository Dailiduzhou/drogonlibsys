use dioxus::prelude::*;

fn main() {
    dioxus::launch(App);
}

#[component]
fn App() -> Element {
    rsx! {
        main {
            class: "shell",
            section {
                class: "hero",
                h1 { "Library Management System" }
                p { "Rust + Dioxus frontend for the Drogon backend." }
            }
        }
    }
}
