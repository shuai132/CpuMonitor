# CpuMonitor

There are two executable file: UI(`CpuMonitor`) and Daemon(`cpu_monitor`).

## UI

### Install Dependencies for Build

* Ubuntu
  ```shell
  sudo apt install -y libgl1-mesa-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
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

## Daemon

### Build and Run

```shell
mkdir build && cd build && cmake .. && make -j8 cpu_monitor
./cpu_monitor -h
```

## Usage

* show help

```shell
./cpu_monitor -h
```

* run cpu_monitor as daemon(server)

```shell
./cpu_monitor -s
```

* open CpuMonitor.app
