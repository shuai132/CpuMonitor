// Prevents additional console window on Windows in release, DO NOT REMOVE!!
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use std::cell::RefCell;
use std::rc::Rc;
use std::sync::{Arc, Mutex};
use std::thread::spawn;

use lazy_static::lazy_static;
use log::info;
use rpc_core::rpc::Rpc;
use rpc_core_net::config_builder;
use rpc_core_net::rpc_client;
use tauri::{Window, WindowEvent};
use tokio::sync::Notify;
use crate::msg_data::MsgData;

mod msg;
mod msg_data;

struct RpcWrap {
    pub rpc: Rc<Rpc>,
}

unsafe impl Send for RpcWrap {}

lazy_static! {
    static ref WINDOW: Mutex<Option<Window>> = Mutex::new(None);
    static ref RPC: Mutex<Option<RpcWrap>> = Mutex::new(None);
    static ref NOTIFY_CLOSE_RPC: Notify = Notify::new();
}

#[tauri::command]
fn init_process(window: Window) {
    *WINDOW.lock().unwrap() = Some(window);
}

fn send_event(event: &str, json: &str) {
    if let Some(w) = WINDOW.lock().unwrap().as_ref() {
        w.emit(event, json).unwrap();
    }
}

#[allow(dead_code)]
fn get_rpc() -> Option<Rc<Rpc>> {
    RPC.lock().unwrap().as_ref().map_or(None, |r| Some(r.rpc.clone()))
}

async fn rpc_task_loop() {
    let rpc = Rpc::new(None);
    let msg_data = Rc::new(RefCell::new(MsgData::default()));

    let msg_data_clone = msg_data.clone();
    rpc.subscribe("on_cpu_msg", move |msg: msg::CpuMsg| {
        msg_data_clone.borrow_mut().process_cpu_msg(msg);
    });

    let msg_data_clone = msg_data.clone();
    rpc.subscribe("on_process_msg", move |msg: msg::ProcessMsg| {
        msg_data_clone.borrow_mut().process_process_msg(msg);
        send_event("on_msg_data", serde_json::to_string(&*msg_data_clone).unwrap().as_str());
    });

    *RPC.lock().unwrap() = Some(RpcWrap { rpc: rpc.clone() });

    let config = config_builder::RpcConfigBuilder::new().rpc(Some(rpc)).build();
    let rpc_client = rpc_client::RpcClient::new(config);
    rpc_client.on_open(|_: Rc<Rpc>| {
        info!("on_open");
    });
    rpc_client.on_open_failed(|e| {
        info!("on_open_failed: {:?}", e);
    });
    rpc_client.on_close(|| {
        info!("on_close");
    });
    rpc_client.set_reconnect(1000);
    rpc_client.open("localhost", 8088);
    info!("rpc running...");

    NOTIFY_CLOSE_RPC.notified().await;
}

fn main() {
    // std::env::set_var("RUST_LOG", "trace");
    env_logger::init();

    let rpc_thread = spawn(|| {
        let runtime = tokio::runtime::Builder::new_current_thread()
            .enable_all()
            .build()
            .unwrap();

        runtime.block_on(async {
            let local = tokio::task::LocalSet::new();
            local.run_until(async move {
                rpc_task_loop().await;
            }).await;
        });
    });

    let rpc_thread = Arc::new(Mutex::new(Some(rpc_thread)));
    let close_rpc = move || {
        NOTIFY_CLOSE_RPC.notify_one();
        rpc_thread.lock().unwrap().take().unwrap().join().unwrap();
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
}
