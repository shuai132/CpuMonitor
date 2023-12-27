use std::cell::RefCell;
use std::rc::Rc;
use std::sync::Arc;

use lazy_static::lazy_static;
use log::{debug, info};
use rpc_core::rpc::Rpc;
use rpc_core_net::config_builder;
use rpc_core_net::rpc_client;
use tauri::Window;
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::sync::{mpsc, Mutex, Notify};

use crate::msg;
use crate::msg_data::MsgData;
use crate::utils::ParsePath;

struct Param {
    cmd: String,
    msg: String,
}

struct RpcChannel {
    tx1: Arc<Mutex<mpsc::Sender<Param>>>,
    rx1: Arc<Mutex<mpsc::Receiver<Param>>>,
    tx2: Arc<Mutex<mpsc::Sender<rpc_core::request::FutureRet<String>>>>,
    rx2: Arc<Mutex<mpsc::Receiver<rpc_core::request::FutureRet<String>>>>,
}

struct CtrlChannel {
    tx1: Arc<Mutex<mpsc::Sender<Param>>>,
    rx1: Arc<Mutex<mpsc::Receiver<Param>>>,
    tx2: Arc<Mutex<mpsc::Sender<Result<String, String>>>>,
    rx2: Arc<Mutex<mpsc::Receiver<Result<String, String>>>>,
}

lazy_static! {
    pub static ref WINDOW: Mutex<Option<Window>> = Mutex::new(None);
    pub static ref NOTIFY_CLOSE_RPC: Notify = Notify::new();
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
    static ref CTRL_CHANNEL: CtrlChannel = {
        let (tx1, rx1) = tokio::sync::mpsc::channel(1);
        let (tx2, rx2) = tokio::sync::mpsc::channel(1);
        CtrlChannel {
            tx1: Arc::new(Mutex::new(tx1)),
            rx1: Arc::new(Mutex::new(rx1)),
            tx2: Arc::new(Mutex::new(tx2)),
            rx2: Arc::new(Mutex::new(rx2)),
        }
    };
}

#[tauri::command]
pub fn init_process(window: Window) {
    *WINDOW.blocking_lock() = Some(window);
}

#[tauri::command]
pub async fn rpc(command: String, message: String) -> Result<String, String> {
    debug!("rpc: cmd: {command}, msg: {message}");
    RPC_CHANNEL.tx1.lock().await.send(Param { cmd: command, msg: message }).await.unwrap();
    let ret = RPC_CHANNEL.rx2.lock().await.recv().await.unwrap();
    debug!("rpc: ret: {ret:?}");
    if let Some(ret) = ret.result {
        Ok(ret)
    } else {
        Err(ret.type_.to_str().to_string())
    }
}

#[tauri::command]
pub async fn ctrl(command: String, message: String) -> Result<String, String> {
    debug!("ctrl: cmd: {command}, msg: {message}");
    CTRL_CHANNEL.tx1.lock().await.send(Param { cmd: command, msg: message }).await.unwrap();
    let ret = CTRL_CHANNEL.rx2.lock().await.recv().await.unwrap();
    debug!("ctrl: ret: {ret:?}");
    ret
}

fn send_event(event: &str, json: &str) {
    WINDOW.try_lock()
        .ok()
        .and_then(|w| w.as_ref().cloned())
        .map(|w| w.emit(event, json).unwrap());
}

async fn save_data_to_file(data: &[u8], path: String) -> tokio::io::Result<String> {
    let path = path.parse_path();
    let mut file = tokio::fs::File::create(path.as_str()).await?;
    file.write_all(data).await?;
    Ok(format!("Save Success: {path}"))
}

async fn load_data_from_file(msg_data: Rc<RefCell<MsgData>>, path: String) -> tokio::io::Result<String> {
    let path = path.parse_path();
    let mut file = tokio::fs::File::open(path.as_str()).await?;
    let mut json_data = String::new();
    file.read_to_string(&mut json_data).await?;
    *msg_data.borrow_mut() = serde_json::from_str::<MsgData>(json_data.as_str())?;
    send_event("on_msg_data", json_data.as_str());
    msg_data.borrow_mut().has_preload_data = true;
    Ok(format!("Load Success: {path}"))
}

fn rpc_message_channel(rpc: Rc<Rpc>, msg_data: Rc<RefCell<MsgData>>, rpc_client: Rc<rpc_client::RpcClient>) {
    tokio::task::spawn_local(async move {
        let mut rx = RPC_CHANNEL.rx1.lock().await;
        loop {
            let param = rx.recv().await.unwrap();
            let result = rpc.cmd(param.cmd).msg(param.msg).future::<String>().await;
            RPC_CHANNEL.tx2.lock().await.send(result).await.unwrap();
        }
    });

    tokio::task::spawn_local(async move {
        let mut rx = CTRL_CHANNEL.rx1.lock().await;
        loop {
            let param = rx.recv().await.unwrap();
            let cmd = param.cmd;
            let msg = param.msg;
            match cmd.as_str() {
                "clear_data" => {
                    msg_data.borrow_mut().clear();
                    CTRL_CHANNEL.tx2.lock().await.send(Ok("ok".to_string())).await.unwrap();
                }
                "save_data" => {
                    let path = msg;
                    let json_data = serde_json::to_string_pretty(&*msg_data).unwrap();
                    let result = save_data_to_file(json_data.as_bytes(), path).await.map_err(|e| e.to_string());
                    CTRL_CHANNEL.tx2.lock().await.send(result).await.unwrap();
                }
                "load_data" => {
                    let path = msg;
                    let result = load_data_from_file(msg_data.clone(), path).await.map_err(|e| e.to_string());
                    CTRL_CHANNEL.tx2.lock().await.send(result).await.unwrap();
                }
                "create_test_data" => {
                    msg_data.borrow_mut().create_test_data();
                    send_event("on_msg_data", serde_json::to_string(&*msg_data).unwrap().as_str());
                    CTRL_CHANNEL.tx2.lock().await.send(Ok("ok".to_string())).await.unwrap();
                }
                "set_ip_addr" => {
                    msg_data.borrow_mut().clear();
                    let addr: Vec<&str> = msg.split(":").collect();
                    if addr.len() != 2 {
                        CTRL_CHANNEL.tx2.lock().await.send(Ok(format!("format error: {msg}"))).await.unwrap();
                    } else {
                        match addr[1].parse::<u16>() {
                            Ok(port) => {
                                rpc_client.open(addr[0], port);
                                CTRL_CHANNEL.tx2.lock().await.send(Ok("ok".to_string())).await.unwrap();
                            }
                            Err(_) => {
                                CTRL_CHANNEL.tx2.lock().await.send(Ok(format!("port invalid: {}", addr[1]))).await.unwrap();
                            }
                        }
                    }
                }
                _ => {
                    CTRL_CHANNEL.tx2.lock().await.send(Err(format!("no such ctrl method: {cmd}"))).await.unwrap();
                }
            }
        }
    });
}

pub async fn rpc_task_loop() {
    let msg_data = Rc::new(RefCell::new(MsgData::default()));
    let msg_data_clone = msg_data.clone();

    let rpc = Rpc::new(None);
    rpc.subscribe("on_cpu_msg", move |msg: msg::CpuMsg| {
        msg_data_clone.borrow_mut().process_cpu_msg(msg);
    });

    let msg_data_clone = msg_data.clone();
    rpc.subscribe("on_process_msg", move |msg: msg::ProcessMsg| {
        if !msg_data_clone.borrow_mut().process_process_msg(msg) {
            return;
        }
        send_event("on_msg_data", serde_json::to_string(&*msg_data_clone).unwrap().as_str());
    });

    let config = config_builder::RpcConfigBuilder::new().rpc(Some(rpc.clone())).build();
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

    rpc_message_channel(rpc, msg_data, rpc_client.clone());
    NOTIFY_CLOSE_RPC.notified().await;
}
