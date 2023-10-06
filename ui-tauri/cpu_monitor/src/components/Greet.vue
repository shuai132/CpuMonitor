<script setup lang="ts">
import {ref} from "vue";
import {invoke} from "@tauri-apps/api/tauri";
import {listen} from "@tauri-apps/api/event";

const cpuJson = ref("");
const processJson = ref("");

invoke('init_process');

listen('on_cpu_msg', (event: any) => {
  cpuJson.value = event.payload.toString();
});

listen('on_process_msg', (event: any) => {
  processJson.value = event.payload.toString();
});

</script>

<template>
  <p>on_cpu_msg: {{ cpuJson }}</p>
  <p>on_process_msg: {{ processJson }}</p>
</template>
