// Prevents additional console window on Windows in release, DO NOT REMOVE!!
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use std::sync::Arc;
use std::thread::spawn;

use tauri::WindowEvent;
use tokio::sync::Mutex;

use crate::rpc_task::*;

mod msg;
mod msg_data;
mod rpc_task;

fn main() {
    std::env::var("RUST_LOG").map_err(|_| {
        std::env::set_var("RUST_LOG", "info");
    }).ok();
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
        rpc_thread.blocking_lock().take().unwrap().join().unwrap();
    };

    tauri::Builder::default()
        .invoke_handler(tauri::generate_handler![init_process, rpc, ctrl])
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
