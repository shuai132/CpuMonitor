<script lang="ts" setup>
import {onMounted, onUnmounted, ref} from 'vue';
import {invoke} from "@tauri-apps/api/tauri";
import {listen} from "@tauri-apps/api/event";
import * as echarts from 'echarts';

let chartCpuAve: echarts.EChartsType;
let chartCpuCores: echarts.EChartsType;
let chartCpuAndMem: echarts.EChartsType[] = [];

let msg_data: any;
let cpu_msg_list: any[];
let process_msg_map: any;

let process_msg_map_ref = ref();
let listen_list: any[] = [];

let ui_config_smooth = false;

const initListen = () => {
  invoke('init_process');

  let un_listen = listen('on_msg_data', (event: any) => {
    const json = event.payload.toString();
    msg_data = JSON.parse(json);
    cpu_msg_list = msg_data["msg_cpus"];

    // ave charts
    chartCpuAve.setOption({
      series: [
        {
          data: cpu_msg_list.map(value => {
            const item = value["ave"];
            return [new Date(item["timestamps"]), item["usage"]]
          }),
          showSymbol: false,
        }
      ]
    });

    // cores charts
    {
      chartCpuCores.setOption({
        legend: {
          data: cpu_msg_list[0]["cores"].map((item: any) => item["name"]),
          top: 26,
        },
        tooltip: {
          trigger: 'axis',
          axisPointer: {
            type: 'cross',
            animation: false,
          },
        },
        series: cpu_msg_list[0]["cores"].map((item: any, i: any) => ({
          type: 'line',
          smooth: ui_config_smooth,
          data: cpu_msg_list.map(value => {
            const item = value["cores"][i];
            return [new Date(item["timestamps"]), item["usage"]];
          }),
          name: item["name"],
          showSymbol: false,
        })),
      });
    }

    process_msg_map = msg_data["msg_pids"];
    updateProcessCharts();
  });
  listen_list.push(un_listen);
};

const updateProcessCharts = () => {
  chartCpuAndMem = [];
  process_msg_map_ref.value = [];
  for (let pid in process_msg_map) {
    process_msg_map_ref.value.push(pid);
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
          // text: "cpu usages",
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
          }
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
        },
        series: item["thread_infos"].map((item: any, index: any) => {
          if (index > 10) {
            return {};
          }
          return {
            type: 'line',
            smooth: ui_config_smooth,
            data: item["cpu_infos"].map((item: any) => {
              return [new Date(item["timestamps"]), item["usage"]];
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
          }
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
            smooth: ui_config_smooth,
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
  chartCpuAve = echarts.init(chartDomCpuAve);
  chartCpuCores = echarts.init(chartDomCpuCore);
  window.onresize = () => {
    chartCpuAve.resize()
    chartCpuCores.resize()
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
      text: 'Cpu Ave Usages',
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
      }
    },
    tooltip: {
      trigger: 'axis',
      axisPointer: {
        type: 'cross',
        animation: false,
      },
      formatter: (p: any) => {
        return `${p[0].data[1].toFixed(2)}%`;
      }
    },
    series: [
      {
        type: 'line',
        smooth: ui_config_smooth,
        areaStyle: {}
      }
    ],
    animation: false,
  });

  chartCpuCores.setOption({
    title: {
      text: 'Cpu Cores Usages',
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
      }
    },
    animation: false,
  });
  echarts.connect([chartCpuAve, chartCpuCores]);
};

onMounted(() => {
  initListen();
  initCharts();
});

onUnmounted(() => {
  listen_list.map((cb: any) => {
    cb();
  })
});
</script>

<template>
  <div id="chart-cpu-ave" class="chart-view"></div>
  <div id="chart-cpu-cores" class="chart-view"></div>
  <div v-for="pid in process_msg_map_ref">
    <div :id="`chart-process-cpu-${pid}`" class="chart-view"></div>
    <div :id="`chart-process-mem-${pid}`" class="chart-view"></div>
  </div>
</template>

<style src="./App.css"></style>
