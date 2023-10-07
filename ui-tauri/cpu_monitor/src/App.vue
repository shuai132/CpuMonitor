<script setup lang="ts">
import {onMounted, onUnmounted} from 'vue';
import {invoke} from "@tauri-apps/api/tauri";
import {listen} from "@tauri-apps/api/event";
import * as echarts from 'echarts';

let chartCpuAve: undefined | echarts.EChartsType = undefined;
let chartCpuCores: undefined | echarts.EChartsType = undefined;

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
    chartCpuAve?.setOption({
      series: [
        {
          data: cpu_msg_list.map(value => {
            const item = value["ave"];
            return [new Date(item["timestamps"]), item["usage"]]
          }),
        }
      ]
    });

    // cores charts
    {
      chartCpuCores?.setOption({
        series: cpu_msg_list[0]["cores"].map((_: any, i: any) => ({
          type: 'line',
          smooth: true,
          data: cpu_msg_list.map(value => {
            const item = value["cores"][i];
            return [new Date(item["timestamps"]), item["usage"]];
          }),
        }))
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
    chartCpuAve?.resize()
    chartCpuCores?.resize()
  };
  {
    let option = {
      xAxis: {
        type: 'time'
      },
      yAxis: {
        type: 'value'
      },
      series: [
        {
          type: 'line',
          smooth: true,
          areaStyle: {}
        }
      ],
      animation: false,
    };

    chartCpuAve.setOption(option);
  }
  {
    let option = {
      xAxis: {
        type: 'time'
      },
      yAxis: {
        type: 'value'
      },
      animation: false,
    };

    chartCpuCores.setOption(option);
  }
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
