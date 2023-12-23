// Prevents additional console window on Windows in release, DO NOT REMOVE!!
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use std::cell::RefCell;
use std::rc::Rc;
use std::sync::Arc;
use std::thread::spawn;

use lazy_static::lazy_static;
use log::{debug, info};
use rpc_core::rpc::Rpc;
use rpc_core_net::config_builder;
use rpc_core_net::rpc_client;
use tauri::{Window, WindowEvent};
use tokio::sync::{mpsc, Mutex, Notify};

use crate::msg_data::MsgData;

mod msg;
mod msg_data;

struct RpcChannel {
    tx1: Arc<Mutex<mpsc::Sender<String>>>,
    rx1: Arc<Mutex<mpsc::Receiver<String>>>,
    tx2: Arc<Mutex<mpsc::Sender<rpc_core::request::FutureRet<String>>>>,
    rx2: Arc<Mutex<mpsc::Receiver<rpc_core::request::FutureRet<String>>>>,
}

lazy_static! {
    static ref WINDOW: Mutex<Option<Window>> = Mutex::new(None);
    static ref NOTIFY_CLOSE_RPC: Notify = Notify::new();
    static ref RPC_CHANNEL: RpcChannel = {
        let (tx1, rx1) = tokio::sync::mpsc::channel(1);
        let (tx2, rx2) = tokio::sync::mpsc::channel(1);
        RpcChannel {
            tx1: Arc::new(Mutex::new(tx1)),
            rx1: Arc::new(Mutex::new(rx1)),
            tx2: Arc::new(Mutex::new(tx2)),
            rx2: Arc::new(Mutex::new(rx2)),
        }
    };
}

#[tauri::command]
fn init_process(window: Window) {
    *WINDOW.blocking_lock() = Some(window);
}

#[tauri::command]
async fn rpc(command: String, message: String) -> Result<String, String> {
    debug!("rpc: cmd: {command}, msg: {message}");
    {
        let tx = RPC_CHANNEL.tx1.lock().await;
        tx.send(command).await.unwrap();
        tx.send(message).await.unwrap();
    }
    let ret = RPC_CHANNEL.rx2.lock().await.recv().await.unwrap();
    debug!("rpc: ret: {ret:?}");
    if let Some(ret) = ret.result {
        Ok(ret)
    } else {
        Err(ret.type_.to_str().to_string())
    }
}

fn send_event(event: &str, json: &str) {
    WINDOW.try_lock()
        .ok()
        .and_then(|w| w.as_ref().cloned())
        .map(|w| w.emit(event, json).unwrap());
}

fn rpc_message_channel(rpc: Rc<Rpc>, msg_data: Rc<RefCell<MsgData>>) {
    let gen_result = |msg: &str| -> rpc_core::request::FutureRet<String> {
        rpc_core::request::FutureRet {
            type_: rpc_core::request::FinallyType::Normal,
            result: Some(msg.to_string()),
        }
    };

    tokio::task::spawn_local(async move {
        let mut rx = RPC_CHANNEL.rx1.lock().await;
        loop {
            let cmd = rx.recv().await.unwrap();
            let msg = rx.recv().await.unwrap();
            match cmd.as_str() {
                "clear_data" => {
                    msg_data.borrow_mut().clear();
                    RPC_CHANNEL.tx2.lock().await.send(gen_result("ok")).await.unwrap();
                }
                _ => {
                    let result = rpc.cmd(cmd).msg(msg).future::<String>().await;
                    RPC_CHANNEL.tx2.lock().await.send(result).await.unwrap();
                }
            }
        }
    });
}

async fn rpc_task_loop() {
    let rpc = Rpc::new(None);
    let msg_data = Rc::new(RefCell::new(MsgData::default()));
    rpc_message_channel(rpc.clone(), msg_data.clone());

    let msg_data_clone = msg_data.clone();
    rpc.subscribe("on_cpu_msg", move |msg: msg::CpuMsg| {
        msg_data_clone.borrow_mut().process_cpu_msg(msg);
    });

    let msg_data_clone = msg_data.clone();
    rpc.subscribe("on_process_msg", move |msg: msg::ProcessMsg| {
        msg_data_clone.borrow_mut().process_process_msg(msg);
        send_event("on_msg_data", serde_json::to_string(&*msg_data_clone).unwrap().as_str());
    });

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
        .invoke_handler(tauri::generate_handler![init_process, rpc])
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
