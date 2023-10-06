<script setup lang="ts">
import {ref} from "vue";
import {onMounted, onUnmounted} from 'vue';
import {invoke} from "@tauri-apps/api/tauri";
import {listen} from "@tauri-apps/api/event";
import * as echarts from 'echarts';
import {EChartsType} from "echarts";

let myChart: undefined | EChartsType = undefined;
const cpuJson = ref("");
const processJson = ref("");

let listen_list: any[] = [];
let cpu_msg_list: any[] = [];
let cpu_msg_list_v: any[] = [];
let process_msg_list: any[] = [];

const initListen = () => {
  invoke('init_process');

  let un_listen = listen('on_cpu_msg', (event: any) => {
    console.log("wtf0: ", event)
    const json = event.payload.toString();
    cpuJson.value = json;
    console.log("wtf1: ", json)
    const jsonObject = JSON.parse(json);
    console.log("wtf2: ", jsonObject)
    cpu_msg_list.push(jsonObject);
    cpu_msg_list_v.push(jsonObject["ave"]["usage"]);
    myChart?.setOption({
      series: [
        {
          data: cpu_msg_list_v,
        }
      ]
    });
  });
  listen_list.push(un_listen);

  un_listen = listen('on_process_msg', (event: any) => {
    const json = event.payload.toString();
    processJson.value = json;
    const jsonObject = JSON.parse(json);
    process_msg_list.push(jsonObject);
  });
  listen_list.push(un_listen);
}

const initCharts = () => {
  const chartDom = document.getElementById('chart');
  myChart = echarts.init(chartDom);
  let option = {
    xAxis: {
      type: 'category'
    },
    yAxis: {
      type: 'value'
    },
    series: [
      {
        data: cpu_msg_list_v,
        type: 'line',
        smooth: true
      }
    ]
  };

  myChart.setOption(option);
  window.onresize = () => {
    myChart?.resize()
  };
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
  <div id="chart" style="display: flex; height: 300px"></div>
  <p>on_cpu_msg: {{ cpuJson }}</p>
  <p>on_process_msg: {{ processJson }}</p>
</template>
