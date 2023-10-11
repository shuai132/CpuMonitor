// Prevents additional console window on Windows in release, DO NOT REMOVE!!
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]
#![recursion_limit = "512"]

use std::ffi::CStr;
use std::sync::Mutex;
use std::thread::spawn;
use cpp::cpp;
use lazy_static::lazy_static;
use tauri::{Window, WindowEvent};

lazy_static! {
    static ref WINDOW: Mutex<Option<Window>> = Mutex::new(None);
}

#[tauri::command]
fn init_process(window: Window) {
    *WINDOW.lock().unwrap() = Some(window);
    unsafe {
        cpp!([]{
            s_context.post([]{
                s_msg.clear();
            });
        })
    }
}

fn send_event(event: &str, json: &str) {
    let w = (*WINDOW).lock().unwrap();
    if w.is_some() {
        w.clone().unwrap().emit(event, json).unwrap();
    }
}

cpp! {{
    #include "asio_net/rpc_client.hpp"
    #include "MsgData.hpp"
    #include "Common.h"
    #include "log.h"

    static MsgData s_msg;
    static std::shared_ptr<rpc_core::rpc> s_rpc;
    static asio::io_context s_context;
}}

fn init_rpc() {
    unsafe {
        cpp!([]{
            using namespace cpu_monitor;
            s_rpc = rpc_core::rpc::create();
            s_rpc->subscribe("on_cpu_msg", [](msg::CpuMsg msg) {
                auto json = nlohmann::json(msg).dump(-1);
                auto str = json.c_str();
                rust!(_on_cpu_msg [str: *const i8 as "const char*"] {
                    send_event("on_cpu_msg", CStr::from_ptr(str).to_str().unwrap());
                });
            });
            s_rpc->subscribe("on_process_msg", [](msg::ProcessMsg msg) {
                s_msg.process(std::move(msg));
                auto msg_pids_json = nlohmann::json(s_msg.msg_pids).dump(-1);
                auto str = msg_pids_json.c_str();
                rust!(_on_process_msg [str: *const i8 as "const char*"] {
                    send_event("on_process_msg", CStr::from_ptr(str).to_str().unwrap());
                });
            });
        })
    }
}

fn run_rpc() {
    unsafe {
        cpp!([]{
            LOG("run rpc...");
            using namespace asio_net;
            rpc_client client(s_context, rpc_config{.rpc = s_rpc});
            client.on_open = [](const std::shared_ptr<rpc_core::rpc>& rpc) {
                (void)rpc;
                LOG("client on_open:");
            };
            client.on_open_failed = [](std::error_code ec) {
                LOG("client on_open_failed: %d, %s", ec.value(), ec.message().c_str());
            };
            client.on_close = [] {
                LOG("client on_close:");
            };
            client.set_reconnect(1000);
            client.open("localhost", 8088);
            client.run();
        })
    }
}

fn main() {
    let rpc_thread = spawn(|| {
        init_rpc();
        run_rpc();
    });

    let close_rpc = || {
        unsafe {
            cpp!([]{
                s_context.stop();
            })
        }
    };

    tauri::Builder::default()
        .invoke_handler(tauri::generate_handler![init_process])
        .on_window_event(move |event| {
            match event.event() {
                WindowEvent::CloseRequested { .. } => {
                    close_rpc();
                }
                _ => {}
            }
        })
        .run(tauri::generate_context!())
        .expect("error while running tauri application");

    rpc_thread.join().unwrap();
}
