use serde::{Deserialize, Serialize};

#[derive(Debug, Default, Serialize, Deserialize)]
pub struct CpuInfo {
    name: String,
    usage: f32,
    timestamps: u64,
}

#[derive(Debug, Default, Serialize, Deserialize)]
pub struct CpuMsg {
    ave: CpuInfo,
    cores: Vec<CpuInfo>,
    timestamps: u64,
}

#[derive(Debug, Default, Serialize, Deserialize)]
pub struct ThreadInfo {
    name: String,
    id: u64,
    usage: f32,
    timestamps: u64,
}

#[derive(Debug, Default, Serialize, Deserialize)]
pub struct MemInfo {
    peak: u64,
    size: u64,
    hwm: u64,
    rss: u64,
    timestamps: u64,
}

#[derive(Debug, Default, Serialize, Deserialize)]
pub struct ProcessInfo {
    id: u64,
    name: String,
    thread_infos: Vec<ThreadInfo>,
    mem_info: MemInfo,
}

#[derive(Debug, Default, Serialize, Deserialize)]
pub struct ProcessMsg {
    infos: Vec<ProcessInfo>,
    timestamps: u64,
}
