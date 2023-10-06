extern crate cpp_build;

fn main() {
    tauri_build::build();
    println!("cargo:rerun-if-changed=src/main.rs");
    cpp_build::Config::new().include("../../../thirdparty/asio_net/include")
        .include("../../../thirdparty/asio_net/include/asio_net/rpc_core/include")
        .include("../../../thirdparty/asio/asio/include")
        .include("../../../thirdparty/asio_net/test")
        .flag("-std=c++14")
        .build("src/main.rs");
}
