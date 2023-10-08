<script setup lang="ts">
import {onMounted, onUnmounted} from 'vue';
import {invoke} from "@tauri-apps/api/tauri";
import {listen} from "@tauri-apps/api/event";
import * as echarts from 'echarts';

let chartCpuAve: echarts.EChartsType;
let chartCpuCores: echarts.EChartsType;

let listen_list: any[] = [];
let cpu_msg_list: any[] = [];
let process_msg_list: any[] = [];

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
    const jsonObject = JSON.parse(json);
    process_msg_list.push(jsonObject);
  });
  listen_list.push(un_listen);
}

const initCharts = () => {
  const chartDomCpuAve = document.getElementById('chart-cpu-ave');
  const chartDomCpuCore = document.getElementById('chart-cpu-cores');
  chartCpuAve = echarts.init(chartDomCpuAve);
  chartCpuCores = echarts.init(chartDomCpuCore);
  window.onresize = () => {
    chartCpuAve.resize()
    chartCpuCores.resize()
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
}

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
</template>
