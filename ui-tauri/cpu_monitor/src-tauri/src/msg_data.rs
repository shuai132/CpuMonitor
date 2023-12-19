#![allow(non_snake_case, unused, private_interfaces)]

use std::cmp::Ordering;
use std::collections::{BTreeMap, HashMap};
use serde::{Deserialize, Serialize};
use crate::msg;
use crate::msg::*;

type ProcessKey = u64;

#[derive(Debug, Default, Serialize, Deserialize)]
struct ProcessValue {
    pub name: String,
    pub threadInfos: Vec<ThreadInfoItem>,
    pub memInfos: Vec<MemInfo>,
    pub maxRss: u64,
}

#[derive(Debug, Serialize, Deserialize)]
struct ThreadInfoKey {
    pub id: u64,
    pub usage: f64,
}

#[derive(Debug, Serialize, Deserialize)]
struct ThreadInfoItem {
    pub key: ThreadInfoKey,
    pub cpuInfos: Vec<ThreadInfo>,
}

impl Eq for ThreadInfoItem {}

impl PartialEq<Self> for ThreadInfoItem {
    fn eq(&self, other: &Self) -> bool {
        self.key.id.eq(&other.key.id)
    }
}

impl PartialOrd<Self> for ThreadInfoItem {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        self.key.id.partial_cmp(&other.key.id)
    }
}

impl Ord for ThreadInfoItem {
    fn cmp(&self, other: &Self) -> Ordering {
        self.key.id.cmp(&other.key.id)
    }
}

#[derive(Debug, Default, Serialize, Deserialize)]
pub struct MsgData {
    pub msg_cpus: Vec<CpuMsg>,
    pub msg_pids: BTreeMap<ProcessKey, ProcessValue>,
    pub pid_current_thread_num: BTreeMap<u64, u32>,
}

impl MsgData {
    pub fn clear(&mut self) {
        self.msg_cpus.clear();
        self.msg_pids.clear();
        self.pid_current_thread_num.clear();
    }

    pub fn create_test_data(&mut self) {
        self.clear();
        for i in 0..1000 {
            let mut msg = CpuMsg::default();
            msg.timestamps = i;
            msg.ave.name = "cpu".to_string();
            msg.ave.usage = ((i as f32 / 10.0).sin() + 1.0) * 50.0;

            for j in 0..4 {
                let mut c = CpuInfo::default();
                c.name = format!("cpu{}", j);
                c.usage = (((i + j * 10) as f32 / 10.0).sin() + 1.0) * 50.0;
                c.timestamps = i;
                msg.cores.push(c);
            }

            self.msg_cpus.push(msg);
        }
    }

    pub fn process_cpu_msg(&mut self, msg: CpuMsg) {
        self.msg_cpus.push(msg);
    }

    pub fn process_process_msg(&mut self, msg: ProcessMsg) {
        for pInfo in msg.infos {
            let processValue = self.msg_pids.entry(pInfo.id).or_insert(ProcessValue::default());
            let threadInfos = &mut processValue.threadInfos;
            self.pid_current_thread_num.insert(pInfo.id, pInfo.thread_infos.len() as u32);

            for item in pInfo.thread_infos {
                if let Some(threadInfo) = threadInfos.iter_mut().find(|v| v.key.id == item.id) {
                    threadInfo.key.usage += item.usage;
                    threadInfo.cpuInfos.push(item);
                } else {
                    let key = ThreadInfoKey {
                        id: item.id,
                        usage: item.usage,
                    };
                    let value = vec![item];
                    threadInfos.push(ThreadInfoItem { key, cpuInfos: value });
                }
            }

            threadInfos.sort();

            processValue.maxRss = processValue.maxRss.max(pInfo.mem_info.rss);
            processValue.memInfos.push(pInfo.mem_info);
        }
    }
}

#[test]
fn test() {
    let mut msg_data = MsgData::default();
    let process_msg_json = r#"{"infos":[{"id":19365,"name":"cpu_monitor","thread_infos":[{"name":"259","id":259,"usage":0.0,"timestamps":1702956002326}],"mem_info":{"peak":0,"size":408695056,"hwm":0,"rss":3584,"timestamps":1702956002326}}],"timestamps":1702956002326}"#;
    let process_msg = serde_json::from_str::<ProcessMsg>(process_msg_json).unwrap();
    msg_data.process_process_msg(process_msg);
    let result = serde_json::to_string(&msg_data);
    println!("msg_data: {}", result.unwrap().as_str());
}
