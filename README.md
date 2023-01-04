# CpuMonitor

## GUI

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
mkdir build && cd build && cmake .. && make CpuMonitor
```

## Monitor

### Build

```shell
mkdir build && cd build && cmake -DBUILD_GUI=OFF .. && make cpu_monitor
```

## Usage

* run cpu_monitor as server

```shell
./cpu_monitor -s
```

* open CpuMonitor.app
