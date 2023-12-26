#![allow(non_snake_case, unused, private_interfaces)]

use std::cmp::Ordering;
use std::collections::{BTreeMap, HashMap};

use serde::{Deserialize, Serialize};

use crate::msg::*;

type ProcessKey = u64;

#[derive(Debug, Default, Serialize, Deserialize)]
struct ProcessValue {
    pub name: String,
    pub thread_infos: Vec<ThreadInfoItem>,
    pub mem_infos: Vec<MemInfo>,
    pub max_rss: u64,
}

type ThreadInfoKey = u64;

#[derive(Debug, Serialize, Deserialize)]
struct ThreadInfoItem {
    pub id: ThreadInfoKey,
    pub usage_sum: f64,
    pub cpu_infos: Vec<ThreadInfo>,
}

#[derive(Debug, Default, Serialize, Deserialize)]
pub struct MsgData {
    pub msg_cpus: Vec<CpuMsg>,
    pub msg_pids: BTreeMap<ProcessKey, ProcessValue>,
    pub pid_current_thread_num: BTreeMap<u64, u32>,

    #[serde(skip_serializing, skip_deserializing)]
    pub has_preload_data: bool,
}

impl MsgData {
    pub fn clear(&mut self) {
        self.msg_cpus.clear();
        self.msg_pids.clear();
        self.pid_current_thread_num.clear();
        self.has_preload_data = false;
    }

    pub fn create_test_data(&mut self) {
        self.clear();
        for i in 0..1000 {
            let mut msg = CpuMsg::default();
            msg.ave.timestamps = i;
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
        self.has_preload_data = true;
    }

    pub fn process_cpu_msg(&mut self, msg: CpuMsg) -> bool {
        if self.has_preload_data {
            return false;
        }
        self.msg_cpus.push(msg);
        true
    }

    pub fn process_process_msg(&mut self, msg: ProcessMsg) -> bool {
        if self.has_preload_data {
            return false;
        }
        for pInfo in msg.infos {
            let processValue = self.msg_pids.entry(pInfo.id).or_insert(ProcessValue::default());
            let threadInfos = &mut processValue.thread_infos;
            self.pid_current_thread_num.insert(pInfo.id, pInfo.thread_infos.len() as u32);

            for item in pInfo.thread_infos {
                if let Some(threadInfo) = threadInfos.iter_mut().find(|v| v.id == item.id) {
                    threadInfo.usage_sum += item.usage;
                    threadInfo.cpu_infos.push(item);
                } else {
                    let id = item.id;
                    let usage_sum = item.usage;
                    let value = vec![item];
                    threadInfos.push(ThreadInfoItem { id, usage_sum, cpu_infos: value });
                }
            }

            threadInfos.sort_unstable_by(|a, b| a.usage_sum.partial_cmp(&b.usage_sum).unwrap_or(Ordering::Equal).reverse());

            processValue.name = pInfo.name;
            processValue.max_rss = processValue.max_rss.max(pInfo.mem_info.rss);
            processValue.mem_infos.push(pInfo.mem_info);
        }
        true
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
