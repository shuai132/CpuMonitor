use serde::{Deserialize, Serialize};

#[derive(Debug, Default, Serialize, Deserialize)]
pub struct CpuInfo {
    pub name: String,
    pub usage: f32,
    pub timestamps: u64,
}

#[derive(Debug, Default, Serialize, Deserialize)]
pub struct CpuMsg {
    pub ave: CpuInfo,
    pub cores: Vec<CpuInfo>,
    pub timestamps: u64,
}

#[derive(Debug, Default, Serialize, Deserialize)]
pub struct ThreadInfo {
    pub name: String,
    pub id: u64,
    pub usage: f64,
    pub timestamps: u64,
}

#[derive(Debug, Default, Serialize, Deserialize)]
pub struct MemInfo {
    pub peak: u64,
    pub size: u64,
    pub hwm: u64,
    pub rss: u64,
    pub timestamps: u64,
}

#[derive(Debug, Default, Serialize, Deserialize)]
pub struct ProcessInfo {
    pub id: u64,
    pub name: String,
    pub thread_infos: Vec<ThreadInfo>,
    pub mem_info: MemInfo,
}

#[derive(Debug, Default, Serialize, Deserialize)]
pub struct ProcessMsg {
    pub infos: Vec<ProcessInfo>,
    pub timestamps: u64,
}
