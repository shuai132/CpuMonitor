use std::cmp::Ordering;
use std::collections::BTreeMap;

use serde::{Deserialize, Serialize};

use crate::msg::*;

type ProcessKey = u64;

#[derive(Debug, Default, Serialize, Deserialize)]
pub struct ProcessValue {
    pub name: String,
    pub thread_infos: Vec<ThreadInfoItem>,
    pub mem_infos: Vec<MemInfo>,
    pub max_rss: u64,
}

type ThreadInfoKey = u64;

#[derive(Debug, Serialize, Deserialize)]
pub struct ThreadInfoItem {
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
            {
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
                self.process_cpu_msg(msg);
            }

            {
                let mut msg = ProcessMsg::default();
                msg.timestamps = i;

                // process
                let mut p = ProcessInfo::default();
                p.id = 100;
                p.name = "app".to_string();

                // threads and mem
                for j in 0..4 {
                    let mut t = ThreadInfo::default();
                    t.id = j;
                    t.name = format!("thread_{}", j);
                    t.usage = (((i + j * 10) as f32 / 10.0).sin() + 1.0) * 50.0;
                    t.timestamps = i;
                    p.thread_infos.push(t);

                    let mut m = MemInfo::default();
                    m.rss = ((((i + j * 10) as f32 / 10.0).sin() + 1.0) * 50.0) as u64 + 1024;
                    m.timestamps = i;
                    p.mem_info = m;
                }
                msg.infos.push(p);
                self.process_process_msg(msg);
            }
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
        for p_info in msg.infos {
            let p_value = self.msg_pids.entry(p_info.id).or_insert(ProcessValue::default());
            let t_infos = &mut p_value.thread_infos;
            self.pid_current_thread_num.insert(p_info.id, p_info.thread_infos.len() as u32);

            for item in p_info.thread_infos {
                if let Some(t_info) = t_infos.iter_mut().find(|v| v.id == item.id) {
                    t_info.usage_sum += item.usage as f64;
                    t_info.cpu_infos.push(item);
                } else {
                    let id = item.id;
                    let usage_sum = item.usage;
                    let value = vec![item];
                    t_infos.push(ThreadInfoItem { id, usage_sum: usage_sum as f64, cpu_infos: value });
                }
            }

            t_infos.sort_unstable_by(|a, b| a.usage_sum.partial_cmp(&b.usage_sum).unwrap_or(Ordering::Equal).reverse());

            p_value.name = p_info.name;
            p_value.max_rss = p_value.max_rss.max(p_info.mem_info.rss);
            p_value.mem_infos.push(p_info.mem_info);
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
