<script setup lang="ts">
import {onMounted, onUnmounted, ref} from 'vue';
import {invoke} from "@tauri-apps/api/tauri";
import {listen} from "@tauri-apps/api/event";
import * as echarts from 'echarts';

let chartCpuAve: echarts.EChartsType;
let chartCpuCores: echarts.EChartsType;
let chartCpuAndMem: echarts.EChartsType[] = [];

let listen_list: any[] = [];
let cpu_msg_list: any[] = [];
let process_msg_list: any = ref();

const initListen = () => {
  invoke('init_process');

  let un_listen = listen('on_cpu_msg', (event: any) => {
    const json = event.payload.toString();
    const jsonObject = JSON.parse(json);
    cpu_msg_list.push(jsonObject);

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
        series: cpu_msg_list[0]["cores"].map((item: any, i: any) => ({
          type: 'line',
          smooth: true,
          data: cpu_msg_list.map(value => {
            const item = value["cores"][i];
            return [new Date(item["timestamps"]), item["usage"]];
          }),
          name: item["name"],
          showSymbol: false,
        })),
      });
    }
  });
  listen_list.push(un_listen);

  un_listen = listen('on_process_msg', (event: any) => {
    const json = event.payload.toString();
    process_msg_list.value = JSON.parse(json);
    updateProcessCharts();
  });
  listen_list.push(un_listen);
};

const updateProcessCharts = () => {
  chartCpuAndMem = [];
  process_msg_list.value.forEach((item: any) => {
    let cpuChart;
    let memChart;
    // cpu
    {
      const chartDom = document.getElementById('chart-process-cpu-' + item[0]['pid']);
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
          text: "name: " + item[0]['name'] + "  pid: " + item[0]['pid'],
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
          data: item[1]["threadInfos"].map((item: any) => item["key"]["id"]),
          top: 26,
          width: "80%",
        },
        series: item[1]["threadInfos"].map((item: any, _: any) => ({
          type: 'line',
          smooth: true,
          data: item["cpuInfos"].map((item: any) => {
            return [new Date(item["timestamps"]), item["usage"]];
          }),
          name: item["key"]["id"],
          showSymbol: false,
        })),
      });
    }

    // mem
    {
      const chartDom = document.getElementById('chart-process-mem-' + item[0]['pid']);
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
          min: 0,
          axisLabel: {
            formatter: '{value}KB'
          }
        },
        animation: false,
        legend: {
          data: ["VmHWM", "VmRSS"],
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
            return (p[1].data[1] / 1024).toFixed(2) + "MB"; // VmRSS
          }
        },
        series: [
          {
            type: 'line',
            smooth: true,
            data: item[1]["memInfos"].map((value: any) => {
              return [new Date(value["timestamps"]), value["hwm"]];
            }),
            name: "VmHWM",
            showSymbol: false,
          },
          {
            type: 'line',
            smooth: true,
            data: item[1]["memInfos"].map((value: any) => {
              return [new Date(value["timestamps"]), value["rss"]];
            }),
            name: "VmRSS",
            showSymbol: false,
          },
        ]
      });
    }

    echarts.connect([cpuChart, memChart]);
  });
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
        return p[0].data[1].toFixed(2) + "%";
      }
    },
    series: [
      {
        type: 'line',
        smooth: true,
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
  <div id="chart-cpu-ave" style="display: flex; height: 280px"></div>
  <div id="chart-cpu-cores" style="display: flex; height: 280px"></div>
  <div v-for="item in process_msg_list">
    <div :id="'chart-process-cpu-' + item[0]['pid']" style="display: flex; height: 280px"></div>
    <div :id="'chart-process-mem-' + item[0]['pid']" style="display: flex; height: 280px"></div>
  </div>
</template>
