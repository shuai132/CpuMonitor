use std::cell::RefCell;
use std::rc::Rc;
use std::sync::Arc;

use lazy_static::lazy_static;
use log::{debug, info};
use rpc_core::net::config_builder;
use rpc_core::net::rpc_client;
use rpc_core::rpc::{Rpc, RpcProto};
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
    let rpc_clone = rpc.clone();
    tokio::task::spawn_local(async move {
        let mut rx = RPC_CHANNEL.rx1.lock().await;
        loop {
            let param = rx.recv().await.unwrap();
            let result = rpc_clone.cmd(param.cmd).msg(param.msg).future::<String>().await;
            RPC_CHANNEL.tx2.lock().await.send(result).await.unwrap();
        }
    });

    tokio::task::spawn_local(async move {
        let mut rx = CTRL_CHANNEL.rx1.lock().await;
        loop {
            let param = rx.recv().await.unwrap();
            let cmd = param.cmd;
            let msg = param.msg;
            let mut result = Ok("ok".to_string());
            match cmd.as_str() {
                "clear_data" => {
                    msg_data.borrow_mut().clear();
                }
                "save_data" => {
                    let path = msg;
                    let json_data = serde_json::to_string_pretty(&*msg_data).unwrap();
                    result = save_data_to_file(json_data.as_bytes(), path).await.map_err(|e| e.to_string());
                }
                "load_data" => {
                    let path = msg;
                    result = load_data_from_file(msg_data.clone(), path).await.map_err(|e| e.to_string());
                }
                "create_test_data" => {
                    msg_data.borrow_mut().create_test_data();
                    send_event("on_msg_data", serde_json::to_string(&*msg_data).unwrap().as_str());
                }
                "get_msg_data" => {
                    send_event("on_msg_data", serde_json::to_string(&*msg_data).unwrap().as_str());
                }
                "fetch_status" => {
                    let status = if rpc.is_ready() { "connected" } else { "disconnected" };
                    send_event("on_status", status);
                    result = Ok(status.to_string());
                }
                "set_ip_addr" => {
                    msg_data.borrow_mut().clear();
                    let addr: Vec<&str> = msg.split(":").collect();
                    if addr.len() != 2 {
                        result = Ok(format!("format error: {msg}"));
                    } else {
                        match addr[1].parse::<u16>() {
                            Ok(port) => {
                                rpc_client.open(addr[0], port);
                            }
                            Err(_) => {
                                result = Ok(format!("port invalid: {}", addr[1]));
                            }
                        }
                    }
                }
                _ => {
                    result = Err(format!("no such ctrl method: {cmd}"));
                }
            }
            CTRL_CHANNEL.tx2.lock().await.send(result).await.unwrap();
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

    let msg_data_clone = msg_data.clone();
    rpc.subscribe("/plugin/malloc", move |msg: msg::PluginMsgMalloc| {
        msg_data_clone.borrow_mut().process_plugin_malloc(msg);
    });

    let msg_data_clone = msg_data.clone();
    rpc.subscribe("/plugin/meminfo", move |msg: msg::PluginMsgMemInfo| {
        msg_data_clone.borrow_mut().process_plugin_mem_info(msg);
    });

    let config = config_builder::RpcConfigBuilder::new().rpc(Some(rpc.clone())).build();
    let rpc_client = rpc_client::RpcClient::new(config);
    rpc_client.on_open(|_: Rc<Rpc>| {
        info!("on_open");
        send_event("on_status", "connected");
    });
    rpc_client.on_open_failed(|e| {
        info!("on_open_failed: {:?}", e);
    });
    rpc_client.on_close(|| {
        info!("on_close");
        send_event("on_status", "disconnected");
    });
    rpc_client.set_reconnect(1000);
    rpc_client.open("localhost", 8088);
    info!("rpc running...");

    rpc_message_channel(rpc, msg_data, rpc_client.clone());
    NOTIFY_CLOSE_RPC.notified().await;
}
