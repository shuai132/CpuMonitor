#pragma once

#include <cstdint>

#ifdef __linux__
#include <bits/types/FILE.h>
#endif

#ifdef __APPLE__
#include <mach/mach_types.h>
#endif

namespace cpu_monitor {

#ifdef __linux__
using CpuInfoNative = FILE;
#endif

#ifdef __APPLE__
using CpuInfoNative = natural_t;
#endif

}  // namespace cpu_monitor
