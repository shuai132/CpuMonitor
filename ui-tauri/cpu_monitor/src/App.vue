<script lang="ts" setup>
import {nextTick, onMounted, onUnmounted, ref} from 'vue';
import {invoke} from "@tauri-apps/api/tauri";
import {listen} from "@tauri-apps/api/event";
import * as echarts from 'echarts';
import {Snackbar} from '@varlet/ui'

let chartCpuAve: echarts.EChartsType;
let chartCpuCores: echarts.EChartsType;
let chartMemInfo: echarts.EChartsType;
let chartCpuAndMem: echarts.EChartsType[] = [];

let msg_data: any;
let cpu_msg_list: any[];
let process_msg_map: any;

let process_msg_map_ref = ref();
let listen_list: any[] = [];

let ui_config_dark = ref(false);
let ui_config_smooth = ref(false);
let ui_config_show_thread_max = ref("10");
let ui_config_mem_info_show_list = ["MemAvailable", "MemFree", "SwapUsed"];
let ui_connect_status = ref("disconnected");

Snackbar.allowMultiple(true)
const toast = Snackbar;

const initListen = () => {
  invoke('init_process');
  ui_get_msg_data();

  let un_listen = listen('on_msg_data', (event: any) => {
    msg_data = JSON.parse(event.payload);
    cpu_msg_list = msg_data["msg_cpus"];

    if (cpu_msg_list.length == 0) {
      return;
    }

    // ave charts
    chartCpuAve.setOption({
      series: [
        {
          type: 'line',
          smooth: ui_config_smooth.value,
          data: cpu_msg_list.map(value => {
            const item = value["ave"];
            return [new Date(item["timestamps"]), item["usage"].toFixed(2)]
          }),
          name: `ave: ${cpu_msg_list.slice(-1)[0]["ave"]["usage"].toFixed(2)}%`,
          showSymbol: false,
        }
      ]
    });

    // cores charts
    {
      chartCpuCores.setOption({
        series: cpu_msg_list.slice(-1)[0]["cores"].map((item: any, i: any) => ({
          type: 'line',
          smooth: ui_config_smooth.value,
          data: cpu_msg_list.map(value => {
            const item = value["cores"][i];
            return [new Date(item["timestamps"]), item["usage"].toFixed(2)];
          }),
          name: `${item["name"]}: ${item["usage"].toFixed(2)}%`,
          showSymbol: false,
        })),
      });
    }

    // mem charts
    {
      let mem_info = msg_data["plugin_mem_info"];
      chartMemInfo.setOption({
        series: Object.keys(mem_info.slice(-1)[0]).filter(name => {
          return ui_config_mem_info_show_list.includes(name)
        }).map((name: any) => (
            {
              type: 'line',
              smooth: ui_config_smooth.value,
              data: mem_info.map((item: any) => {
                return [new Date(item["timestamps"]), item[name].toFixed(2)];
              }),
              name: `${name}`,
              showSymbol: false,
            })),
      });
    }

    process_msg_map = msg_data["msg_pids"];
    updateProcessChartsDom();
    nextTick().then(() => {
      updateProcessCharts();
    });
  });
  listen_list.push(un_listen);

  listen_list.push(listen('on_status', (event: any) => {
    ui_connect_status.value = event.payload;
    if (event.payload == "connected") {
      check_version();
    }
  }));
};

const updateProcessChartsDom = () => {
  chartCpuAndMem = [];
  process_msg_map_ref.value = [];
  for (let pid in process_msg_map) {
    process_msg_map_ref.value.push(pid);
  }
}

const updateProcessCharts = () => {
  for (let pid in process_msg_map) {
    let item = process_msg_map[pid];
    let cpuChart;
    let memChart;
    // cpu
    {
      const chartDom = document.getElementById(`chart-process-cpu-${pid}`);
      if (!chartDom) return;
      let chart: echarts.EChartsType | undefined = echarts.getInstanceByDom(chartDom);
      if (!chart) {
        chart = echarts.init(chartDom);
      }

      cpuChart = chart;
      chartCpuAndMem.push(chart);
      const dataZoomOption = [
        {
          show: true,
          realtime: true,
          start: 0,
          end: 100,
        },
        {
          type: 'inside',
          realtime: true,
          start: 0,
          end: 100,
          zoomLock: true,
        }
      ];
      chart.setOption({
        title: {
          text: `${item['name']}  pid: ${pid}  threads: ${msg_data["pid_current_thread_num"][pid]}`,
          textStyle: {
            fontSize: 15,
          },
          left: 'center',
        },
        xAxis: {
          type: 'time'
        },
        dataZoom: dataZoomOption,
        yAxis: {
          type: 'value',
          min: 0,
          max: 100,
          axisLabel: {
            formatter: '{value}%'
          },
        },
        animation: false,
        legend: {
          top: 26,
          width: "80%",
          // need `data` for sort
          data: item["thread_infos"].slice(0, parseInt(ui_config_show_thread_max.value)).map((item: any, _: any) => {
            return `${item["id"]}: ${item["cpu_infos"][0]["name"]}`;
          }),
        },
        tooltip: {
          trigger: 'axis',
          axisPointer: {
            type: 'cross',
            animation: false,
          },
        },
        series: item["thread_infos"].slice(0, parseInt(ui_config_show_thread_max.value)).map((item: any, _: any) => {
          return {
            type: 'line',
            smooth: ui_config_smooth.value,
            data: item["cpu_infos"].map((item: any) => {
              return [new Date(item["timestamps"]), item["usage"].toFixed(2)];
            }),
            name: `${item["id"]}: ${item["cpu_infos"][0]["name"]}`,
            showSymbol: false,
          };
        }),
      });
    }

    // mem
    {
      const chartDom = document.getElementById(`chart-process-mem-${pid}`);
      if (!chartDom) return;
      let chart: echarts.EChartsType | undefined = echarts.getInstanceByDom(chartDom);
      if (!chart) {
        chart = echarts.init(chartDom);
      }

      memChart = chart;
      chartCpuAndMem.push(chart);
      const dataZoomOption = [
        {
          show: true,
          realtime: true,
          start: 0,
          end: 100,
        },
        {
          type: 'inside',
          realtime: true,
          start: 0,
          end: 100,
          zoomLock: true,
        }
      ];
      chart.setOption({
        title: {
          text: "memory usages",
          textStyle: {
            fontSize: 15,
          },
          left: 'center',
        },
        xAxis: {
          type: 'time'
        },
        dataZoom: dataZoomOption,
        yAxis: {
          type: 'value',
          min: 'dataMin',
          max: 'dataMax',
          axisLabel: {
            formatter: '{value}KB'
          },
        },
        animation: false,
        legend: {
          top: 26,
          width: "80%",
        },
        tooltip: {
          trigger: 'axis',
          axisPointer: {
            type: 'cross',
            animation: false,
          },
          formatter: (p: any) => {
            let value = p[0].data[1];
            let value_mb = (value / 1024).toFixed(2);
            return `${value_mb}MB(${value}KB)`; // VmRSS
          }
        },
        series: [
          {
            type: 'line',
            smooth: ui_config_smooth.value,
            data: item["mem_infos"].map((value: any) => {
              return [new Date(value["timestamps"]), value["rss"]];
            }),
            name: (() => {
              let rss_now = item["mem_infos"].slice(-1)[0]["rss"];
              let rss_now_mb = (rss_now / 1024).toFixed(2);
              let rss_max = process_msg_map[pid]["max_rss"];
              let rss_max_mb = (rss_max / 1024).toFixed(2);
              return `VmRSS: ${rss_now_mb}MB(${rss_now}KB) MAX:${rss_max_mb}MB(${rss_max}KB)`;
            })(),
            showSymbol: false,
          },
        ]
      });
    }

    echarts.connect([cpuChart, memChart]);
  }
};

const initCharts = () => {
  const chartDomCpuAve = document.getElementById('chart-cpu-ave');
  const chartDomCpuCore = document.getElementById('chart-cpu-cores');
  const chartDomMemInfo = document.getElementById('chart-mem-info');
  chartCpuAve = echarts.init(chartDomCpuAve);
  chartCpuCores = echarts.init(chartDomCpuCore);
  chartMemInfo = echarts.init(chartDomMemInfo);
  window.onresize = () => {
    chartCpuAve.resize()
    chartCpuCores.resize()
    chartMemInfo.resize()
    chartCpuAndMem.forEach(item => item.resize());
  };
  const dataZoomOption = [
    {
      show: true,
      realtime: true,
      start: 0,
      end: 100,
    },
    {
      type: 'inside',
      realtime: true,
      start: 0,
      end: 100,
      zoomLock: true,
    }
  ];
  chartCpuAve.setOption({
    title: {
      text: 'Cpu Ave Usage',
      textStyle: {
        fontSize: 15,
      },
      left: 'center',
    },
    xAxis: {
      type: 'time',
    },
    dataZoom: dataZoomOption,
    yAxis: {
      type: 'value',
      min: 0,
      max: 100,
      axisLabel: {
        formatter: '{value}%'
      },
    },
    legend: {
      top: 26,
    },
    tooltip: {
      trigger: 'axis',
      axisPointer: {
        type: 'cross',
        animation: false,
      },
      formatter: (p: any) => {
        return `${p[0].data[1]}%`;
      }
    },
    animation: false,
  });

  chartCpuCores.setOption({
    title: {
      text: 'Cpu Cores Usage',
      textStyle: {
        fontSize: 15,
      },
      left: 'center',
    },
    xAxis: {
      type: 'time'
    },
    dataZoom: dataZoomOption,
    yAxis: {
      type: 'value',
      min: 0,
      max: 100,
      axisLabel: {
        formatter: '{value}%'
      },
    },
    legend: {
      top: 26,
    },
    tooltip: {
      trigger: 'axis',
      axisPointer: {
        type: 'cross',
        animation: false,
      },
      formatter: (p: any) => {
        return p.map((v: any) => {
          return `${v.marker}${v.seriesName.slice(0, -5)} ${v.value[1]}%`
        }).join("<br>");
      }
    },
    animation: false,
  });

  chartMemInfo.setOption({
    title: {
      text: 'Memory Usage',
      textStyle: {
        fontSize: 15,
      },
      left: 'center',
    },
    xAxis: {
      type: 'time'
    },
    dataZoom: dataZoomOption,
    yAxis: {
      type: 'value',
      axisLabel: {
        formatter: '{value} kB'
      },
    },
    legend: {
      top: 26,
    },
    tooltip: {
      trigger: 'axis',
      axisPointer: {
        type: 'cross',
        animation: false,
      },
    },
    animation: false,
  });

  echarts.connect([chartCpuAve, chartCpuCores, chartMemInfo]);
};

function check_version() {
  invoke('rpc', {
    command: "get_version",
    message: "",
  }).then((result: any) => {
    ui_connect_status.value += `: ${result}`
    toast.success("cpu_monitor version: " + result);
  }).catch((_: any) => {
  });
}

onMounted(() => {
  initListen();
  initCharts();
  ui_fetch_status();
  ui_get_msg_data();
});

onUnmounted(() => {
  listen_list.map((cb: any) => {
    cb();
  })
});

/**** ui settings ****/
let ui_add_name_text = ref("");
let ui_add_pid_text = ref("");
let ui_save_path_text = ref("");
let ui_load_path_text = ref("");
let ui_addr_ip_text = ref("localhost");
let ui_addr_port_text = ref("8088");

const ui_popup_settings = ref(false)
const ui_button_color = "#3c78c8";

const ui_clear_button = () => {
  chartCpuAve.setOption({
    series: [{data: [],}]
  });
  chartCpuCores.setOption({
    series: cpu_msg_list.slice(-1)[0]["cores"].map(() => ({
      data: [],
    })),
  });
  chartMemInfo.setOption({
    series: ui_config_mem_info_show_list.map(() => ({
      data: [],
    })),
  });
  process_msg_map_ref.value = [];

  invoke('ctrl', {
    command: "clear_data",
    message: "",
  }).then((result: any) => {
    toast.success(result);
  }).catch((reason: any) => {
    toast.error(reason);
  });
};

const ui_create_test_data_button = () => {
  invoke('ctrl', {
    command: "create_test_data",
    message: "",
  }).then((result: any) => {
    toast.success(result);
  }).catch((reason: any) => {
    toast.error(reason);
  });
};

const ui_save_button = () => {
  invoke('ctrl', {
    command: "save_data",
    message: ui_save_path_text.value,
  }).then((result: any) => {
    toast.success(result);
  }).catch((reason: any) => {
    toast.error(reason);
  });
};

const ui_load_button = () => {
  invoke('ctrl', {
    command: "load_data",
    message: ui_load_path_text.value,
  }).then((result: any) => {
    toast.success(result);
  }).catch((reason: any) => {
    toast.error(reason);
  });
};

const ui_set_ip_addr = () => {
  invoke('ctrl', {
    command: "set_ip_addr",
    message: `${ui_addr_ip_text.value}:${ui_addr_port_text.value}`,
  }).then((result: any) => {
    toast.success(result);
  }).catch((reason: any) => {
    toast.error(reason);
  });
};

const ui_add_name_button = () => {
  invoke('rpc', {
    command: "add_name",
    message: ui_add_name_text.value,
  }).then((result: any) => {
    toast.success(result);
  }).catch((reason: any) => {
    toast.error(reason);
  });
};

const ui_del_name_button = () => {
  invoke('rpc', {
    command: "del_name",
    message: ui_add_name_text.value,
  }).then((result: any) => {
    toast.success(result);
  }).catch((reason: any) => {
    toast.error(reason);
  });
};

const ui_add_pid_button = () => {
  invoke('rpc', {
    command: "add_pid",
    message: ui_add_pid_text.value,
  }).then((result: any) => {
    toast.success(result);
  }).catch((reason: any) => {
    toast.error(reason);
  });
};

const ui_del_pid_button = () => {
  invoke('rpc', {
    command: "del_pid",
    message: ui_add_pid_text.value,
  }).then((result: any) => {
    toast.success(result);
  }).catch((reason: any) => {
    toast.error(reason);
  });
};

const ui_fetch_status = () => {
  return invoke('ctrl', {
    command: "fetch_status",
    message: "",
  });
};

const ui_get_msg_data = () => {
  invoke('ctrl', {
    command: "get_msg_data",
    message: "",
  });
};

</script>

<template>
  <!--  menu button  -->
  <div style="display: flex; flex-direction: column; position: fixed; bottom: 30px; right: 20px; z-index: 10;">
    <var-button class="action" round type="success">
      <var-icon name="delete" size="30px" @click="ui_clear_button"/>
    </var-button>
    <span style="height: 8px"></span>
    <var-button class="action" round type="success">
      <var-icon name="menu" size="30px" @click="ui_popup_settings = !ui_popup_settings"/>
    </var-button>
  </div>

  <!--  chart view  -->
  <div>
    <div id="chart-cpu-ave" class="chart-view"></div>
    <div id="chart-cpu-cores" class="chart-view"></div>
    <div id="chart-mem-info" class="chart-view"></div>

    <div v-for="pid in process_msg_map_ref">
      <var-divider/>
      <div :id="`chart-process-cpu-${pid}`" class="chart-view"></div>
      <div :id="`chart-process-mem-${pid}`" class="chart-view"></div>
    </div>
  </div>

  <!--  chart view  -->
  <var-popup
      v-model:show="ui_popup_settings"
      :overlay-style="{backgroundColor: 'rgba(0, 0, 0, 0)'}"
      autofocus="autofocus"
      position="right"
      style="width: 280px; padding: 15px"
  >

    <var-app-bar color="linear-gradient(90deg, rgba(72,176,221,1) 0%, rgba(0,208,161,1) 100%)"
                 style="margin-bottom: 30px" title="Settings"
                 title-position="center">
      <template #left>
        <var-button
            color="transparent"
            round
            text
            text-color="#fff"
        >
          <var-icon :size="24" name="chevron-left" @click="ui_popup_settings = !ui_popup_settings"/>
        </var-button>
      </template>

      <template #right>
        <var-menu>
          <var-button
              color="transparent"
              round
              text
              text-color="#fff"
          >
            <var-icon :size="24" name="menu"/>
          </var-button>
        </var-menu>
      </template>
    </var-app-bar>

    <var-divider description="UI"/>

    <var-space justify="space-between">
      <var-space :size="[0, 10]">
        <var-icon v-if="ui_config_dark" class="ui-setting-icon" name="weather-night" size="26"/>
        <var-icon v-else class="ui-setting-icon" name="white-balance-sunny" size="26"/>
        <span style="font-size: 18px; line-height: 30px; color: #808080;">Dark</span>
      </var-space>
      <var-switch v-model="ui_config_dark" :color=ui_button_color :size="25"
                  @click='toast.error("not yet implemented")'/>
    </var-space>

    <var-divider/>

    <var-space justify="space-between">
      <var-space :size="[0, 10]">
        <img v-if="ui_config_smooth" alt="" class="ui-setting-icon" src="/smooth_on.svg">
        <img v-else alt="" class="ui-setting-icon" src="/smooth_off.svg">
        <span style="font-size: 18px; line-height: 30px; color: #808080;">Smooth</span>
      </var-space>
      <var-switch v-model="ui_config_smooth" :color=ui_button_color :size="25" @click='ui_get_msg_data'/>
    </var-space>

    <var-divider/>

    <var-space justify="space-between">
      <var-space :size="[0, 10]">
        <img alt="" class="ui-setting-icon" src="/thread.svg">
        <span style="font-size: 18px; line-height: 30px; color: #808080;">Show Threads</span>
      </var-space>
      <var-input v-model="ui_config_show_thread_max"
                 placeholder="max"
                 size="small"
                 style="width: 60px; height: 0"
                 type="number"
                 variant="outlined"/>
    </var-space>

    <var-divider/>
    <var-divider description="monitor"/>

    <div class="ui-setting-div">
      <var-input v-model="ui_add_name_text"
                 class="ui-setting-input"
                 placeholder="Name"
                 size="small"
                 type="text"
                 variant="outlined"/>
      <span class="ui-setting-span"></span>
      <var-button :color=ui_button_color class="ui-setting-button" type="primary"
                  @click="ui_add_name_button">Add
      </var-button>
      <span class="ui-setting-span"></span>
      <var-button :color=ui_button_color class="ui-setting-button" type="primary"
                  @click="ui_del_name_button">Del
      </var-button>
    </div>

    <var-divider/>

    <div class="ui-setting-div">
      <var-input v-model="ui_add_pid_text"
                 class="ui-setting-input"
                 placeholder="PID"
                 size="small"
                 type="text"
                 variant="outlined"/>
      <span class="ui-setting-span"></span>
      <var-button :color=ui_button_color class="ui-setting-button" type="primary"
                  @click="ui_add_pid_button">Add
      </var-button>
      <span class="ui-setting-span"></span>
      <var-button :color=ui_button_color class="ui-setting-button" type="primary"
                  @click="ui_del_pid_button">Del
      </var-button>
    </div>

    <var-divider description="save / load record"/>

    <div class="ui-setting-div">
      <var-input v-model="ui_save_path_text"
                 class="ui-setting-input-path"
                 placeholder="File Name"
                 size="small"
                 type="text"
                 variant="outlined"/>
      <span class="ui-setting-span"></span>
      <var-button :color=ui_button_color class="ui-setting-button" type="primary"
                  @click="ui_save_button">Save
      </var-button>
    </div>

    <var-divider/>

    <div class="ui-setting-div">
      <var-input v-model="ui_load_path_text"
                 class="ui-setting-input-path"
                 placeholder="File Name"
                 size="small"
                 type="text"
                 variant="outlined"/>
      <span class="ui-setting-span"></span>
      <var-button :color=ui_button_color class="ui-setting-button" type="primary"
                  @click="ui_load_button">Load
      </var-button>
    </div>

    <var-divider/>
    <var-divider :description="ui_connect_status"/>

    <div class="ui-setting-div">
      <var-input v-model="ui_addr_ip_text"
                 placeholder="IP"
                 size="small"
                 style="width: 132px"
                 type="text"
                 variant="outlined"/>
      <span class="ui-setting-span"></span>
      <var-input v-model="ui_addr_port_text"
                 placeholder="Port"
                 size="small"
                 style="width: 75px"
                 type="text"
                 variant="outlined"/>
      <span class="ui-setting-span"></span>
      <var-button :color=ui_button_color class="ui-setting-button" type="primary"
                  @click="ui_set_ip_addr">Set
      </var-button>
    </div>

    <var-divider/>
    <var-divider description="other"/>

    <div class="ui-setting-div">
      <var-button :color="ui_button_color" class="ui-setting-button" style="width: 257px" type="primary"
                  @click="ui_create_test_data_button">
        Create Test Data
      </var-button>
    </div>
    <var-divider/>

    <div class="ui-setting-div">
      <var-button :color="ui_button_color" class="ui-setting-button" style="width: 257px" type="primary"
                  @click="ui_clear_button">
        Clear History Data
      </var-button>
    </div>

    <var-divider/>
  </var-popup>

</template>

<style src="./App.css"></style>
