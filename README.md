# CpuMonitor

[![Build Status](https://github.com/shuai132/CpuMonitor/workflows/macOS/badge.svg)](https://github.com/shuai132/CpuMonitor/actions?workflow=macOS)
[![Build Status](https://github.com/shuai132/CpuMonitor/workflows/Ubuntu/badge.svg)](https://github.com/shuai132/CpuMonitor/actions?workflow=Ubuntu)
[![Build Status](https://github.com/shuai132/CpuMonitor/workflows/Windows/badge.svg)](https://github.com/shuai132/CpuMonitor/actions?workflow=Windows)

The main aims of this project is to monitor `RAM of process` and `CPU usage of each thread`,
and draw their `curve graphs`.

There are two executable file: Daemon(`cpu_monitor`) and UI(`CpuMonitor`).  
They work in a C/S mode. Daemon program implements all functions except for drawing the curve graph, making it
possible to use on restricted platforms. UI can run on any platform.

## Daemon

support `Linux` and `macOS`

### Build and Run

```shell
mkdir build && cd build && cmake .. && make -j8 cpu_monitor
./cpu_monitor -h
```

### Usage

Note: `macOS` need root permission

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

# FAQs

* Connect via USB ADB:
  ```shell
  adb forward tcp:8088 tcp:8088
  ```
