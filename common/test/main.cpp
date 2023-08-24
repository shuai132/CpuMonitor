#include "CpuMsg_generated.h"
#include "RpcMsg.h"
#include "assert_def.h"

using namespace cpu_monitor;

int main() {
  std::string payload;
  {
    msg::CpuMsg msg;
    for (int i = 0; i < 3; ++i) {
      auto cpuInfo = std::make_unique<msg::CpuInfo>();
      cpuInfo->name = "cpu" + std::to_string(i);
      cpuInfo->usage = 1.f + (float)i;
      msg->cores.push_back(std::move(cpuInfo));
    }

    payload = msg.serialize();
  }
  {
    msg::CpuMsg msg;
    bool ok = msg.deSerialize(payload);
    ASSERT(ok);
    for (const auto& item : msg->cores) {
      printf("name: %s, usage: %.2f\n", item->name.c_str(), item->usage);
    }
  }
}
