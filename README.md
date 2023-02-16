# CpuMonitor

there are two executable file, UI(`CpuMonitor`) and Daemon(`cpu_monitor`).

## UI

### Install Dependencies for Build

* Ubuntu

```shell
sudo apt install -y libgl1-mesa-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

* macOS  
  Nothing todo

* Windows(MinGW)  
  Nothing todo

### Build

```shell
mkdir build && cd build && cmake .. && make -j8
```

## Daemon

### Build

```shell
mkdir build && cd build && cmake .. && make cpu_monitor
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
