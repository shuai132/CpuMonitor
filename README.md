# CpuMonitor

[![Build Status](https://github.com/shuai132/CpuMonitor/workflows/macOS/badge.svg)](https://github.com/shuai132/CpuMonitor/actions?workflow=macOS)
[![Build Status](https://github.com/shuai132/CpuMonitor/workflows/Ubuntu/badge.svg)](https://github.com/shuai132/CpuMonitor/actions?workflow=Ubuntu)
[![Build Status](https://github.com/shuai132/CpuMonitor/workflows/Windows/badge.svg)](https://github.com/shuai132/CpuMonitor/actions?workflow=Windows)

The main aims of this project is to monitor the `RAM of the process` and the `CPU usage of each thread` of this
process, and draw their `curve graphs`.

There are two executable file: UI(`CpuMonitor`) and Daemon(`cpu_monitor`).  
They work in a C/S mode. The daemon program implements all functions except for drawing the curve graph, making it
possible to use on restricted platforms.

## Daemon

support `Linux` and `macOS`

### Build and Run

```shell
mkdir build && cd build && cmake .. && make -j8 cpu_monitor
./cpu_monitor -h
```

### Usage

Notice: `macOS` need root permission

* show help

```shell
./cpu_monitor -h
```

* run cpu_monitor as daemon(server)

```shell
./cpu_monitor -s
```

## UI

support `Linux` and `macOS` and `Windows`

### Install Dependencies for Build

* Ubuntu
  ```shell
  sudo apt-get install -y libsdl2-dev
  ```

* macOS  
  `nothing todo`


* Windows(MinGW)  
  `nothing todo`

### Build and Run

```shell
mkdir build && cd build && cmake .. && make -j8 CpuMonitor
./CpuMonitor
```
